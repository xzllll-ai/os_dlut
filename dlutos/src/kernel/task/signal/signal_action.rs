use super::{KResult, SAVED_DATA_SIZE};
use crate::{
    io::BufferFill as _,
    kernel::{
        constants::EFAULT,
        syscall::UserMut,
        user::UserBuffer,
    },
};
use alloc::{collections::btree_map::BTreeMap, sync::Arc};
use core::arch::naked_asm;
use dlutos_hal::{fpu::FpuState, traits::trap::RawTrapContext, trap::TrapContext};
use dlutos_mm::address::{Addr as _, AddrOps as _, VAddr};
use dlutos_sync::Spin;
use posix_types::{
    ctypes::Long,
    signal::{SigAction, SigActionHandler, SigActionRestorer, SigSet, Signal, TryFromSigAction},
    SIGNAL_NOW,
};

#[unsafe(naked)]
#[unsafe(link_section = ".vdso.rt_sigreturn")]
unsafe extern "C" fn vdso_rt_sigreturn() {
    naked_asm!(
        "li a7, {sys_rt_sigreturn}",
        "ecall",
        sys_rt_sigreturn = const posix_types::syscall_no::SYS_RT_SIGRETURN,
    );
}

#[derive(Debug, Clone, Copy)]
pub enum SignalAction {
    Default,
    Ignore,
    SimpleHandler {
        handler: SigActionHandler,
        restorer: Option<SigActionRestorer>,
        mask: SigSet,
        siginfo: bool,
    },
}

#[derive(Debug)]
pub struct SignalActionList {
    actions: Spin<BTreeMap<Signal, SignalAction>>,
}

impl SignalActionList {
    pub fn new_shared(other: &Arc<Self>) -> Arc<Self> {
        other.clone()
    }

    pub fn new_cloned(other: &Self) -> Arc<Self> {
        Arc::new(Self {
            actions: Spin::new(other.actions.lock().clone()),
        })
    }
}

impl SignalActionList {
    pub const fn new() -> Self {
        Self {
            actions: Spin::new(BTreeMap::new()),
        }
    }

    pub fn set(&self, signal: Signal, action: SignalAction) {
        debug_assert!(
            !matches!(signal, SIGNAL_NOW!()),
            "SIGSTOP and SIGKILL should not be set for a handler."
        );
        match action {
            SignalAction::Default => self.actions.lock().remove(&signal),
            _ => self.actions.lock().insert(signal, action),
        };
    }

    pub fn get(&self, signal: Signal) -> SignalAction {
        match self.actions.lock().get(&signal) {
            None => SignalAction::Default,
            Some(action) => action.clone(),
        }
    }

    pub fn remove_non_ignore(&self) {
        // Remove all custom handlers except for the ignore action.
        // Default handlers should never appear in the list so we don't consider that.
        self.actions
            .lock()
            .retain(|_, action| matches!(action, SignalAction::Ignore));
    }
}

impl SignalAction {
    /// # Might Sleep
    pub(super) fn handle(
        self,
        signal: Signal,
        old_mask: SigSet,
        trap_ctx: &mut TrapContext,
        fpu_state: &mut FpuState,
    ) -> KResult<()> {
        let SignalAction::SimpleHandler {
            handler,
            restorer,
            siginfo,
            ..
        } = self
        else {
            unreachable!("Default and Ignore actions should not be handled here");
        };

        let current_sp = VAddr::from(trap_ctx.get_stack_pointer());

        let saved_data_addr = (current_sp - SAVED_DATA_SIZE).floor_to(16);

        let mut saved_data_buffer =
            UserBuffer::new(UserMut::new(saved_data_addr), SAVED_DATA_SIZE)?;

        saved_data_buffer.copy(trap_ctx)?.ok_or(EFAULT)?;
        saved_data_buffer.copy(fpu_state)?.ok_or(EFAULT)?;
        saved_data_buffer.copy(&old_mask)?.ok_or(EFAULT)?;

        let return_address = if let Some(restorer) = restorer {
            restorer.addr().addr()
        } else {
            {
                static VDSO_RT_SIGRETURN_ADDR: &'static unsafe extern "C" fn() =
                    &(vdso_rt_sigreturn as unsafe extern "C" fn());

                unsafe {
                    // SAFETY: To prevent the compiler from optimizing this into `la` instructions
                    //         and causing a linking error.
                    (VDSO_RT_SIGRETURN_ADDR as *const _ as *const usize).read_volatile()
                }
            }
        };

        let args = [Long::new_val(signal.into_raw() as _).get(), 0, 0];
        let args = &args[..if siginfo { 3 } else { 1 }];

        trap_ctx.set_user_call_frame(
            handler.addr().addr(),
            Some(saved_data_addr.addr()),
            Some(return_address),
            args,
            |vaddr, data| -> Result<(), u32> {
                let mut buffer = UserBuffer::new(UserMut::new(vaddr), data.len())?;
                for ch in data.iter() {
                    buffer.copy(&ch)?.ok_or(EFAULT)?;
                }

                Ok(())
            },
        )?;

        Ok(())
    }
}

impl Clone for SignalActionList {
    fn clone(&self) -> Self {
        Self {
            actions: Spin::new(self.actions.lock().clone()),
        }
    }
}

impl Default for SignalAction {
    fn default() -> Self {
        Self::Default
    }
}

impl TryFromSigAction for SignalAction {
    type Error = u32;

    fn default() -> Self {
        Self::Default
    }

    fn ignore() -> Self {
        Self::Ignore
    }

    fn new() -> Self {
        Self::SimpleHandler {
            handler: SigActionHandler::null(),
            restorer: None,
            mask: SigSet::empty(),
            siginfo: false,
        }
    }

    fn set_siginfo(self) -> Result<Self, Self::Error> {
        if let Self::SimpleHandler {
            handler,
            restorer,
            mask,
            ..
        } = self
        {
            Ok(Self::SimpleHandler {
                handler,
                restorer,
                mask,
                siginfo: true,
            })
        } else {
            unreachable!()
        }
    }

    fn handler(mut self, new_handler: SigActionHandler) -> Self {
        if let Self::SimpleHandler { handler, .. } = &mut self {
            *handler = new_handler;
            self
        } else {
            unreachable!()
        }
    }

    fn restorer(mut self, new_restorer: SigActionRestorer) -> Self {
        if let Self::SimpleHandler { restorer, .. } = &mut self {
            *restorer = Some(new_restorer);
            self
        } else {
            unreachable!()
        }
    }

    fn mask(mut self, new_mask: SigSet) -> Self {
        if let Self::SimpleHandler { mask, .. } = &mut self {
            *mask = new_mask;
            self
        } else {
            unreachable!()
        }
    }
}

impl From<SignalAction> for SigAction {
    fn from(action: SignalAction) -> SigAction {
        match action {
            SignalAction::Default => SigAction::default(),
            SignalAction::Ignore => SigAction::ignore(),
            SignalAction::SimpleHandler {
                handler,
                restorer,
                mask,
                ..
            } => {
                let action = SigAction::new().handler(handler).mask(mask);

                if let Some(restorer) = restorer {
                    action.restorer(restorer)
                } else {
                    action
                }
            }
        }
    }
}

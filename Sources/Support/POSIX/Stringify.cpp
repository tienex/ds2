//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Support/Stringify.h"
#include "DebugServer2/Support/StringifyPrivate.h"
#include "DebugServer2/Utils/Log.h"

#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ptrace.h>

namespace ds2 {
namespace Support {
namespace POSIX {

char const *Stringify::Signal(int signal) {
#if !defined(OS_DARWIN)
  // SIGRTMIN can expand to a glibc call (not usable in a switch statement), so
  // check for it first.
  if (signal == SIGRTMIN)
    return "SIGRTMIN";
#endif

  switch (signal) {
    DO_STRINGIFY(SIGABRT)
    DO_STRINGIFY(SIGALRM)
    DO_STRINGIFY(SIGBUS)
    DO_STRINGIFY(SIGCHLD)
    DO_STRINGIFY(SIGCONT)
    DO_STRINGIFY(SIGFPE)
    DO_STRINGIFY(SIGHUP)
    DO_STRINGIFY(SIGILL)
    DO_STRINGIFY(SIGINT)
    DO_STRINGIFY(SIGIO)
    DO_STRINGIFY(SIGKILL)
    DO_STRINGIFY(SIGPIPE)
    DO_STRINGIFY(SIGPROF)
    DO_STRINGIFY(SIGQUIT)
    DO_STRINGIFY(SIGSEGV)
    DO_STRINGIFY(SIGSTOP)
    DO_STRINGIFY(SIGSYS)
    DO_STRINGIFY(SIGTERM)
    DO_STRINGIFY(SIGTRAP)
    DO_STRINGIFY(SIGTSTP)
    DO_STRINGIFY(SIGTTIN)
    DO_STRINGIFY(SIGTTOU)
    DO_STRINGIFY(SIGURG)
    DO_STRINGIFY(SIGUSR1)
    DO_STRINGIFY(SIGUSR2)
    DO_STRINGIFY(SIGVTALRM)
    DO_STRINGIFY(SIGWINCH)
    DO_STRINGIFY(SIGXCPU)
    DO_STRINGIFY(SIGXFSZ)
#if defined(OS_LINUX)
    DO_STRINGIFY(SIGPWR)
    DO_STRINGIFY(SIGSTKFLT)
#endif
    DO_DEFAULT("unknown signal", signal)
  }
}

char const *Stringify::SignalCode(int signal, int code) {
  switch (signal) {
  case SIGILL:
    switch (code) {
      DO_STRINGIFY(ILL_ILLOPC)
      DO_STRINGIFY(ILL_ILLOPN)
      DO_STRINGIFY(ILL_ILLADR)
      DO_STRINGIFY(ILL_ILLTRP)
      DO_STRINGIFY(ILL_PRVOPC)
      DO_STRINGIFY(ILL_PRVREG)
      DO_STRINGIFY(ILL_COPROC)
      DO_STRINGIFY(ILL_BADSTK)
      DO_DEFAULT("unknown code", code)
    };

  case SIGBUS:
    switch (code) {
      DO_STRINGIFY(BUS_ADRALN)
      DO_STRINGIFY(BUS_ADRERR)
      DO_STRINGIFY(BUS_OBJERR)
      DO_DEFAULT("unknown code", code)
    };

  case SIGSEGV:
    switch (code) {
      DO_STRINGIFY(SEGV_MAPERR)
      DO_STRINGIFY(SEGV_ACCERR)
      DO_DEFAULT("unknown code", code)
    };

    DO_DEFAULT("unknown signal", signal)
  }
}

char const *Stringify::Errno(int error) {
  switch (error) {
    DO_STRINGIFY(E2BIG)
    DO_STRINGIFY(EACCES)
    DO_STRINGIFY(EADDRINUSE)
    DO_STRINGIFY(EADDRNOTAVAIL)
    DO_STRINGIFY(EAFNOSUPPORT)
    DO_STRINGIFY(EAGAIN)
    DO_STRINGIFY(EALREADY)
    DO_STRINGIFY(EBADF)
    DO_STRINGIFY(EBADMSG)
    DO_STRINGIFY(EBUSY)
    DO_STRINGIFY(ECANCELED)
    DO_STRINGIFY(ECHILD)
    DO_STRINGIFY(ECONNABORTED)
    DO_STRINGIFY(ECONNREFUSED)
    DO_STRINGIFY(ECONNRESET)
    DO_STRINGIFY(EDEADLK)
    DO_STRINGIFY(EDESTADDRREQ)
    DO_STRINGIFY(EDOM)
    DO_STRINGIFY(EDQUOT)
    DO_STRINGIFY(EEXIST)
    DO_STRINGIFY(EFAULT)
    DO_STRINGIFY(EFBIG)
    DO_STRINGIFY(EHOSTDOWN)
    DO_STRINGIFY(EHOSTUNREACH)
    DO_STRINGIFY(EIDRM)
    DO_STRINGIFY(EILSEQ)
    DO_STRINGIFY(EINPROGRESS)
    DO_STRINGIFY(EINTR)
    DO_STRINGIFY(EINVAL)
    DO_STRINGIFY(EIO)
    DO_STRINGIFY(EISCONN)
    DO_STRINGIFY(EISDIR)
    DO_STRINGIFY(ELOOP)
    DO_STRINGIFY(EMFILE)
    DO_STRINGIFY(EMLINK)
    DO_STRINGIFY(EMSGSIZE)
    DO_STRINGIFY(EMULTIHOP)
    DO_STRINGIFY(ENAMETOOLONG)
    DO_STRINGIFY(ENETDOWN)
    DO_STRINGIFY(ENETRESET)
    DO_STRINGIFY(ENETUNREACH)
    DO_STRINGIFY(ENFILE)
    DO_STRINGIFY(ENOBUFS)
    DO_STRINGIFY(ENODEV)
    DO_STRINGIFY(ENOENT)
    DO_STRINGIFY(ENOEXEC)
    DO_STRINGIFY(ENOLCK)
    DO_STRINGIFY(ENOLINK)
    DO_STRINGIFY(ENOMEM)
    DO_STRINGIFY(ENOMSG)
    DO_STRINGIFY(ENOPROTOOPT)
    DO_STRINGIFY(ENOSPC)
    DO_STRINGIFY(ENOSYS)
    DO_STRINGIFY(ENOTBLK)
    DO_STRINGIFY(ENOTCONN)
    DO_STRINGIFY(ENOTDIR)
    DO_STRINGIFY(ENOTEMPTY)
    DO_STRINGIFY(ENOTRECOVERABLE)
    DO_STRINGIFY(ENOTSOCK)
    DO_STRINGIFY(ENOTTY)
    DO_STRINGIFY(ENXIO)
    DO_STRINGIFY(EOPNOTSUPP)
    DO_STRINGIFY(EOVERFLOW)
    DO_STRINGIFY(EOWNERDEAD)
    DO_STRINGIFY(EPERM)
    DO_STRINGIFY(EPFNOSUPPORT)
    DO_STRINGIFY(EPIPE)
    DO_STRINGIFY(EPROTO)
    DO_STRINGIFY(EPROTONOSUPPORT)
    DO_STRINGIFY(EPROTOTYPE)
    DO_STRINGIFY(ERANGE)
    DO_STRINGIFY(EREMOTE)
    DO_STRINGIFY(EROFS)
    DO_STRINGIFY(ESHUTDOWN)
    DO_STRINGIFY(ESOCKTNOSUPPORT)
    DO_STRINGIFY(ESPIPE)
    DO_STRINGIFY(ESRCH)
    DO_STRINGIFY(ESTALE)
    DO_STRINGIFY(ETIMEDOUT)
    DO_STRINGIFY(ETOOMANYREFS)
    DO_STRINGIFY(ETXTBSY)
    DO_STRINGIFY(EUSERS)
    DO_STRINGIFY(EXDEV)
#if defined(OS_LINUX)
    DO_STRINGIFY(EADV)
    DO_STRINGIFY(EBADE)
    DO_STRINGIFY(EBADFD)
    DO_STRINGIFY(EBADR)
    DO_STRINGIFY(EBADRQC)
    DO_STRINGIFY(EBADSLT)
    DO_STRINGIFY(EBFONT)
    DO_STRINGIFY(ECHRNG)
    DO_STRINGIFY(ECOMM)
    DO_STRINGIFY(EDOTDOT)
    DO_STRINGIFY(EHWPOISON)
    DO_STRINGIFY(EISNAM)
    DO_STRINGIFY(EKEYEXPIRED)
    DO_STRINGIFY(EKEYREJECTED)
    DO_STRINGIFY(EKEYREVOKED)
    DO_STRINGIFY(EL2HLT)
    DO_STRINGIFY(EL2NSYNC)
    DO_STRINGIFY(EL3HLT)
    DO_STRINGIFY(EL3RST)
    DO_STRINGIFY(ELIBACC)
    DO_STRINGIFY(ELIBBAD)
    DO_STRINGIFY(ELIBEXEC)
    DO_STRINGIFY(ELIBMAX)
    DO_STRINGIFY(ELIBSCN)
    DO_STRINGIFY(ELNRNG)
    DO_STRINGIFY(EMEDIUMTYPE)
    DO_STRINGIFY(ENAVAIL)
    DO_STRINGIFY(ENOANO)
    DO_STRINGIFY(ENOCSI)
    DO_STRINGIFY(ENODATA)
    DO_STRINGIFY(ENOKEY)
    DO_STRINGIFY(ENOMEDIUM)
    DO_STRINGIFY(ENONET)
    DO_STRINGIFY(ENOPKG)
    DO_STRINGIFY(ENOSR)
    DO_STRINGIFY(ENOSTR)
    DO_STRINGIFY(ENOTNAM)
    DO_STRINGIFY(ENOTUNIQ)
    DO_STRINGIFY(EREMCHG)
    DO_STRINGIFY(EREMOTEIO)
    DO_STRINGIFY(ERESTART)
    DO_STRINGIFY(ERFKILL)
    DO_STRINGIFY(ESRMNT)
    DO_STRINGIFY(ESTRPIPE)
    DO_STRINGIFY(ETIME)
    DO_STRINGIFY(EUCLEAN)
    DO_STRINGIFY(EUNATCH)
    DO_STRINGIFY(EXFULL)
#endif
    DO_DEFAULT("unknown error", error)
  }
}

char const *Stringify::Ptrace(int code) {
  switch (code) {
#if defined(OS_LINUX)
    // On CentOS, PTRACE_* are enums, and can't
    // be used inside define statements. CentOS
    // defines PT_* macros, which can be used instead
    DO_STRINGIFY(PTRACE_ATTACH)
    DO_STRINGIFY(PTRACE_CONT)
    DO_STRINGIFY(PTRACE_DETACH)
    DO_STRINGIFY(PTRACE_GETEVENTMSG)
#if defined(PTRACE_GETHBPREGS) || defined(PT_GETHBPREGS)
    DO_STRINGIFY(PTRACE_GETHBPREGS)
#endif
#if defined(PTRACE_GETREGS) || defined(PT_GETREGS)
    DO_STRINGIFY(PTRACE_GETREGS)
#endif
#if defined(PTRACE_GETFPREGS) || defined(PT_GETFPREGS)
    DO_STRINGIFY(PTRACE_GETFPREGS)
#endif
    DO_STRINGIFY(PTRACE_GETREGSET)
    DO_STRINGIFY(PTRACE_GETSIGINFO)
#if defined(PTRACE_GETVFPREGS) || defined(PT_GETVFPREGS)
    DO_STRINGIFY(PTRACE_GETVFPREGS)
#endif
    DO_STRINGIFY(PTRACE_INTERRUPT)
    DO_STRINGIFY(PTRACE_KILL)
    DO_STRINGIFY(PTRACE_LISTEN)
    DO_STRINGIFY(PTRACE_PEEKDATA)
    DO_STRINGIFY(PTRACE_PEEKTEXT)
    DO_STRINGIFY(PTRACE_PEEKUSER)
    DO_STRINGIFY(PTRACE_POKEDATA)
    DO_STRINGIFY(PTRACE_POKETEXT)
    DO_STRINGIFY(PTRACE_POKEUSER)
    DO_STRINGIFY(PTRACE_SEIZE)
#if defined(PTRACE_SETHBPREGS) || defined(PT_SETHBPREGS)
    DO_STRINGIFY(PTRACE_SETHBPREGS)
#endif
    DO_STRINGIFY(PTRACE_SETOPTIONS)
#if defined(PTRACE_SETREGS) || defined(PT_SETREGS)
    DO_STRINGIFY(PTRACE_SETREGS)
#endif
#if defined(PTRACE_SETFPREGS) || defined(PT_SETFPREGS)
    DO_STRINGIFY(PTRACE_SETFPREGS)
#endif
    DO_STRINGIFY(PTRACE_SETREGSET)
    DO_STRINGIFY(PTRACE_SETSIGINFO)
#if defined(PTRACE_SETVFPREGS) || defined(SETVFPREGS)
    DO_STRINGIFY(PTRACE_SETVFPREGS)
#endif
    DO_STRINGIFY(PTRACE_SINGLESTEP)
    DO_STRINGIFY(PTRACE_SYSCALL)
    DO_STRINGIFY(PTRACE_TRACEME)
#endif
    DO_DEFAULT("unknown ptrace command", code)
  }
}
}
}
}

#include "assert_macro.h"

#include <sam.h>

#include "dcc_stdio.h"

// provides debugger only assertion support
// we are providing the routine called by the standard assert macro in assert.h
// if NDEBUG is defined (for release code) the macro does *not* call this
// function, it's a no-op NOTE: the standard definition has a no_return
// attribute which we are *not* doing; we breakpoint but can continue execution
//       we'll live with this one warning...
void __assert_func(
    const char *fileName, int lineNum, const char *caller,
    const char *expression
)
{
    // don't really need any of this printing as the debugger shows all of this
    // as part of the SIGTRAP information
    dbg_write_str("Assertion '");
    dbg_write_str(expression);
    dbg_write_str("' failed in ");
    dbg_write_str(fileName);
    dbg_write_char(':');
    dbg_write_str(caller);
    dbg_write_char('(');
    // this could be more robust; just printing the hex value is easier but less
    // useful
    dbg_write_char(lineNum / 1000 + '0');
    dbg_write_char((lineNum / 100) % 10 + '0');
    dbg_write_char((lineNum / 10) % 10 + '0');
    dbg_write_char(lineNum % 10 + '0');
    dbg_write_char(')');

    // for debugging purposes *this* is all the code we really need
    // don't breakpoint if we're not running via a debugger, otherwise we stop
    // 'forever'
    if ((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) ==
        CoreDebug_DHCSR_C_DEBUGEN_Msk)
        __BKPT(0);
}

/* 
 * File:   WDT.h
 * Author: chtacklind
 *
 * Created on January 25, 2012, 11:33 AM
 */

#ifndef WDT_H
#define	WDT_H

#include <avr/wdt.h>

#undef AVR

namespace AVR {

class WDT {
public:
    enum timeout_e {
        T0015ms = 0,
        T0030ms = 1,
        T0060ms = 2,
        T0120ms = 3,
        T0250ms = 4,
        T0500ms = 5,
        T1000ms = 6,
        T2000ms = 7,
        T4000ms = 8,
        T8000ms = 9
    };
    
    /**
     * Starts the watchdog timer with timeout t
     * @param t the timeout to wait for
     */
    static inline void start(timeout_e t) {wdt_enable(t);}
    
    /**
     * Resets the WDT (call this more often than the timeout!)
     */
    static inline void tick() {wdt_reset();}
    
    /**
     * Stops the WDT. May not always succeed as WDT can be forced on by fuses
     */
    static inline void stop() {wdt_disable();}
    
    static inline bool isEnabled()          {return bit_is_set(WDTCSR, WDE);}
    static inline bool isInterruptEnabled() {return bit_is_set(WDTCSR, WDIE);}
    static inline void clearResetFlag()     {MCUSR = ~_BV(WDRF);}
};

};

#endif	/* WDT_H */


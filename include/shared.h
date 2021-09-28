#define S_P 91.0
#define S_I 0.26
#define S_D 7950.0
#define S_aP 100.0
#define S_aI 0.0
#define S_aD 0.0
#define S_TSET 99
#define S_TBAND 1.5

// PID - offline values
#define AGGKP 25                   // Kp normal
#define AGGTN 250                  // Tn
#define AGGTV 0                    // Tv

// PID coldstart
#define STARTKP 35                 // Start Kp during coldstart
#define STARTTN 130                // Start Tn during cold start

// PID - offline brewdetection values
#define AGGBKP 70                  // Kp
#define AGGBTN 0                   // Tn 
#define AGGBTV 20                  // Tv

#define BUF_SIZE 1024

extern unsigned long time_now;
extern unsigned long time_last;
extern double gTargetTemp;
extern double gOvershoot;
extern double gP;
extern double gI;
extern double gD;
extern double gaP;
extern double gaI;
extern double gaD;

extern int machineState;
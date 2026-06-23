#ifndef BSP_EM7028_H
#define BSP_EM7028_H

#include "em7028.h"

#ifdef __cplusplus
extern "C" {
#endif

int BSP_EM7028_Init(void);
int BSP_EM7028_Probe(void);
int BSP_EM7028_Start(void);
int BSP_EM7028_Stop(void);
int BSP_EM7028_ReadHrs1Data(uint16_t * value);
int BSP_EM7028_ReadRegister(uint8_t reg, uint8_t * value);
uint8_t BSP_EM7028_GetLastPid(void);
int BSP_EM7028_GetLastProbeStatus(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_EM7028_H */

#ifndef _ARM_H
#define _ARM_H

/* ==== PAR_EL1 related ==== */
#define PAR_FAILED(par) (par & 1)
#define PAR_PA(par) (par & 0x0000fffffffff000)

/* ==== ESR_EL1 related ==== */
#define EC_SVC_64       0x15
#define EC_IA_LE        0x20
#define EC_DA_LE        0x24

#define ISS_FSC(esr) (esr->iss & 0x3f)

#define FSC_TF_L0       0b000100
#define FSC_TF_L1       0b000101
#define FSC_TF_L2       0b000110
#define FSC_TF_L3       0b000111

#define ISS_WnR(esr) (esr->iss & 0x40)

typedef struct {
    unsigned int iss:25, // Instruction specific syndrome
                 il:1,   // Instruction length bit
                 ec:6;   // Exception class
} esr_el1_t;

#endif /* _ARM_H */
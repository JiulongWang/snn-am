#include "snn.h"
#include "snn_portme.h"

int main(){
    #if !__DEBUG__
    uint32_t n = 100;
    #endif
    #if __NUP_TEST__
    printf("Neural without Time Stemp Intr test\n");
    us16x4_t taulrvr = {0, 0, 4, 0};
    uint32_t tau = 4;
    __rv_svr(taulrvr, 0);
    us16x4_t nu_without_ts = {0, 0, 0, 0};
    us16x4_t s_without_ts = {100, 100, 100, 100};
    uint16_t nu_without_ts_ref[4] = {0, 0, 0, 0};
    for(int i = 0; i < n; i++){
        nu_without_ts = __rv_nup(nu_without_ts, s_without_ts, 0);
        for (int j = 0; j < 4; j++){
            nu_without_ts_ref[j] = nu_without_ts_ref[j] + (s_without_ts[j] >> tau) - (nu_without_ts_ref[j] >> tau);
            
            if(nu_without_ts[j] != nu_without_ts_ref[j]){
                FAIL_MSG;
                printf("%x \n", nu_without_ts[j]);
                printf("%x \n", nu_without_ts_ref[j]);
                return 0;
            }
        }
    }
    SUCCESS_MSG;
    printf("Neural with Time Stemp Intr test\n");
    us16x4_t nu_with_ts = {0, 0, 0, 0};
    us16x4_t s_with_ts = {100, 100, 100, 100};
    // us16x4_t s_with_ts = {0, 0, 0, 0};
    uint16_t nu_withTS_ref[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    // nu_with_ts = __rv_nup(nu_with_ts, s_with_ts, 1);
    // printf("%x %x %x %x\n", nu_with_ts[0], nu_with_ts[1], nu_with_ts[2], nu_with_ts[3]);
    for (int i = 0; i < n; i++){
        nu_with_ts = __rv_nup(nu_with_ts, s_with_ts, 1);
        for(int j = 0; j < 4; j++){
            nu_withTS_ref[j * 2 + 1] = nu_withTS_ref[j * 2 + 1] + (s_with_ts[j] >> tau) - (nu_withTS_ref[j * 2 + 1] >> tau);
            if(nu_with_ts[j] != nu_withTS_ref[j * 2 + 1] + (nu_withTS_ref[j * 2] << 8)){
            FAIL_MSG;
            printf("%x \t", nu_with_ts[j]);
            printf("%x %x %x\n", nu_withTS_ref[j], nu_withTS_ref[j *2 +1], nu_withTS_ref[j * 2 + 1] + (nu_withTS_ref[j] << 8));
            return 0;
            }
        }    
    }
    SUCCESS_MSG;
    #endif

    #if __SUM_TEST__
    printf("sum test without acc register\n");
    us16x4_t a = {1, 1, 1, 1}; 
    us16x4_t b = {1, 1, 1, 1}; 
    uint64_t sum_without_acc_res = 0;
    uint64_t sum_without_res_ref = 0;
    for(int i = 0; i < n; i++){
        sum_without_acc_res = __rv_sum(a, b, 0);
        for(int j = 0; j < 4; j++){
            sum_without_res_ref = b[j] == 1? sum_without_res_ref + a[j] : sum_without_res_ref;
        }
        if(sum_without_acc_res != sum_without_res_ref){
            FAIL_MSG;
            printf("dut %x ref %x\n", sum_without_acc_res, sum_without_res_ref);
        }
        sum_without_res_ref = 0;
    }
    SUCCESS_MSG;

    printf("sum test with acc register\n");
    __rv_svr((us16x4_t){0, 0, 0, 0}, 0);
    uint64_t sum_with_acc_res = 0;
    uint64_t sum_with_res_ref = 0;
    for(int i = 0; i < n; i++){
       sum_with_acc_res = __rv_sum(a, b, 1);
    //    sum_with_acc_res = __rv_sum(a, b, 1);
       for(int j = 0; j < 4; j++){
           sum_with_res_ref = b[j] == 1? sum_with_res_ref + a[j] : sum_with_res_ref;
        }
        if(sum_with_acc_res != sum_with_res_ref){
            FAIL_MSG;
            printf("dut %x ref %x\n", sum_with_acc_res, sum_with_res_ref);
            return 0;
        }
    }

    SUCCESS_MSG;
    #endif

    #if __TDR_TEST__
    printf("time stamp test\n");
    us16x4_t nu0 = {0x1f00, 0x1e00, 0x1d00, 0x1c00};
    us16x4_t tdr_res = {0, 0, 0, 0};
    uint16_t nu_tdr_ref[4] = {0, 0, 0, 0};
    for (int i = 0; i < n; i++){
        us16x4_t nu1 = {i << 8, i <<8, i <<8, i << 8};
        tdr_res = __rv_tdr(nu0, nu1);
        for(int j = 0; j < 4; j++){
            nu_tdr_ref[j] = (nu0[j] - nu1[j]) >> 8;
            if(tdr_res[j] != nu_tdr_ref[j]){
            FAIL_MSG;
            printf("j = %d %x \n", j , tdr_res[j]);
            printf("%x \n", nu_tdr_ref[j]);
            return 0;
            }
        }
    }
    SUCCESS_MSG;
    #endif

    #if __BP_TEST__
    printf("bp-stdp output layer xi calc test\n");
    srand(20230523);
    us16x4_t xi = {0, 0, 0, 0};
    for(int i = 0; i < n; i++){
        us16x4_t sp03 = {rand() %2, rand() %2, rand() %2, rand() %2};
        us16x4_t tar = {rand() %2, rand() %2 ,rand() %2, rand() %2};
        xi = __rv_bpo(sp03, tar);
        for(int j = 0; j < 4; j++){
            if(xi[j] != (tar[j] == 1? (sp03[j] == 1? 0 : 1) : (sp03[j] == 1 ? 0xffff : 0))){
                printf("sp %x tar %x xi %x\n", sp03[j], tar[j], xi[j]);
                FAIL_MSG;
                return 0;
            }
        }
    }
    SUCCESS_MSG;
    #endif
    #if __EXP_TEST__
    printf("EXP test:\n");
    us16x4_t e = {WEIGHT_FIX_POINT(0), WEIGHT_FIX_POINT(0.6), WEIGHT_FIX_POINT(0.2), WEIGHT_FIX_POINT(1)};
    us16x4_t exp_res = __rv_exp(e);
    printf("%x %x %x %x\n", exp_res[0], exp_res[1], exp_res[2], exp_res[3]);
    #endif

    return 0;
}





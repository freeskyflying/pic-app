/* JSON_checker.h */
#ifndef cJSON_CHECKER_h
#define cJSON_CHECKER_h

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct JSON_checker_struct {
    int valid;
    int state;
    int depth;
    int top;
    int* stack;
} * JSON_checker;


extern JSON_checker new_JSON_checker(int depth);
extern int  JSON_checker_char(JSON_checker jc, int next_char);
extern int  JSON_checker_done(JSON_checker jc);

#ifdef __cplusplus
}
#endif

#endif

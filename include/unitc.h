#ifndef UNITC_H
#define UNITC_H

#define test(test) \
  { \
    printf("Test '%s'\n", #test); \
    test_reset(); \
    init(); \
    test_reset(); \
    test(); \
    if(!test_asserted()) \
    { \
      printf("[%s:%s:%d] There was no assertion\n", __FILE__, __FUNCTION__, __LINE__); \
      test_fail(); \
    } \
    test_commit(); \
    test_reset(); \
    cleanup(); \
  }

#define assert_equals(a ,b) printf("[%s:%s:%d] Assert %s == %s ... ", __FILE__, __FUNCTION__, __LINE__, #a, #b); if(a == b) { printf("OK.\n"); } else { printf("Failed.\n"); assert_fail(); } assert_add();
#define assert_different(a ,b) printf("[%s:%s:%d] Assert %s != %s ... ", __FILE__, __FUNCTION__, __LINE__, #a, #b); if(a != b) { printf("OK.\n"); } else { printf("Failed.\n"); assert_fail(); } assert_add();
#define assert_equals_int(a ,b) printf("[%s:%s:%d] Assert %s == %s ... ", __FILE__, __FUNCTION__, __LINE__, #a, #b); if(a == b) { printf("OK.\n"); } else { printf("Failed (%d, %d).\n", a, b); assert_fail(); } assert_add();
#define assert_different_int(a ,b) printf("[%s:%s:%d] Assert %s != %s ... ", __FILE__, __FUNCTION__, __LINE__, #a, #b); if(a != b) { printf("OK.\n"); } else { printf("Failed (%d, %d).\n", a, b); assert_fail(); } assert_add();
#define assert_equals_uint(a ,b) printf("[%s:%s:%d] Assert %s == %s ... ", __FILE__, __FUNCTION__, __LINE__, #a, #b); if(a == b) { printf("OK.\n"); } else { printf("Failed (%u, %u).\n", a, b); assert_fail(); } assert_add();
#define assert_different_uint(a ,b) printf("[%s:%s:%d] Assert %s != %s ... ", __FILE__, __FUNCTION__, __LINE__, #a, #b); if(a != b) { printf("OK.\n"); } else { printf("Failed (%u, %u).\n", a, b); assert_fail(); } assert_add();
#define assert_equals_float(a ,b) printf("[%s:%s:%d] Assert %s == %s ... ", __FILE__, __FUNCTION__, __LINE__, #a, #b); if(a == b) { printf("OK.\n"); } else { printf("Failed (%f, %f).\n", a, b); assert_fail(); } assert_add();
#define assert_different_float(a ,b) printf("[%s:%s:%d] Assert %s != %s ... ", __FILE__, __FUNCTION__, __LINE__, #a, #b); if(a != b) { printf("OK.\n"); } else { printf("Failed (%f, %f).\n", a, b); assert_fail(); } assert_add();
#define assert_equals_double(a ,b) printf("[%s:%s:%d] Assert %s == %s ... ", __FILE__, __FUNCTION__, __LINE__, #a, #b); if(a == b) { printf("OK.\n"); } else { printf("Failed (%F, %F).\n", a, b); assert_fail(); } assert_add();
#define assert_different_double(a ,b) printf("[%s:%s:%d] Assert %s != %s ... ", __FILE__, __FUNCTION__, __LINE__, #a, #b); if(a != b) { printf("OK.\n"); } else { printf("Failed (%F, %F).\n", a, b); assert_fail(); } assert_add();
#define assert_equals_ptr(a ,b) printf("[%s:%s:%d] Assert %s == %s ... ", __FILE__, __FUNCTION__, __LINE__, #a, #b); if(a == b) { printf("OK.\n"); } else { printf("Failed (%z, %z).\n", a, b); assert_fail(); } assert_add();
#define assert_different_ptr(a ,b) printf("[%s:%s:%d] Assert %s != %s ... ", __FILE__, __FUNCTION__, __LINE__, #a, #b); if(a != b) { printf("OK.\n"); } else { printf("Failed (%z, %z).\n", a, b); assert_fail(); } assert_add();
#define assert_equals_ptrdiff(a ,b) printf("[%s:%s:%d] Assert %s == %s ... ", __FILE__, __FUNCTION__, __LINE__, #a, #b); if(a == b) { printf("OK.\n"); } else { printf("Failed (%t, %t).\n", a, b); assert_fail(); } assert_add();
#define assert_different_ptrdiff(a ,b) printf("[%s:%s:%d] Assert %s != %s ... ", __FILE__, __FUNCTION__, __LINE__, #a, #b); if(a != b) { printf("OK.\n"); } else { printf("Failed (%t, %t).\n", a, b); assert_fail(); } assert_add();
#define assert_true(a) printf("[%s:%s:%d] Assert %s == TRUE ... ", __FILE__, __FUNCTION__, __LINE__, #a); if(a) { printf("OK.\n"); } else { printf("Failed.\n"); assert_fail(); } assert_add();
#define assert_false(a) printf("[%s:%s:%d] Assert %s == FALSE ... ", __FILE__, __FUNCTION__, __LINE__, #a); if(!a) { printf("OK.\n"); } else { printf("Failed.\n"); assert_fail(); } assert_add();
#define assert(a) assert_true(a)

void init(); // Before each test
void cleanup(); // After each test
void setup(); // Before each test suite
void teardown(); // After each test suite
void run(); 

void test_commit();
void test_fail();
void test_reset();
int test_passed();
int test_asserted();
void assert_add();
void assert_fail();
#endif

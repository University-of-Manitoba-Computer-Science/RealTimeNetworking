#ifndef ASSERT_MACRO_H_
#define ASSERT_MACRO_H_

void __assert_func(
    const char *filename, int lineNum, const char *caller,
    const char *expression
);

#endif // ASSERT_MACRO_H_

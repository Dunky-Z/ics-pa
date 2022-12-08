/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
    TK_NOTYPE = 256,
    TK_EQ,
    TK_HEX,
    TK_NUM,
    TK_REG,

    /* TODO: Add more token types */

};

static struct rule {
    const char *regex;
    int         token_type;
} rules[] = {

    /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
    /**
   * 在此添加更多的token
   * 注意不同规则的优先级。
   */

    { " +", TK_NOTYPE },               // spaces
    { "==", TK_EQ },                   // equal
    { "0x[0-9a-fA-F]{1,16}", TK_HEX }, // hex
    { "[0-9]{1,10}", TK_NUM },         // dec
    { "\\$[a-z0-9]{1,31}", TK_REG },   // register names
    { "\\+", '+' },                    // +
    { "-", '-' },                      // -
    { "\\*", '*' },                    // *
    { "/", '/' },                      // /
    { "%", '%' }
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex()
{
    int  i;
    char error_msg[128];
    int  ret;

    for (i = 0; i < NR_REGEX; i++) {
        ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
        if (ret != 0) {
            regerror(ret, &re[i], error_msg, 128);
            panic("regex compilation failed: %s\n%s", error_msg,
                  rules[i].regex);
        }
    }
}

typedef struct token {
    int  type;
    char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int   nr_token __attribute__((used))   = 0;

/**
 * token 就是表达式中的各个元素，比如3+2中的token胡原始股'3' '+' '2' 
 * 给出一个待求值表达式, 我们首先要识别出其中的token, 进行这项工作的是make_token()函数
 */
static bool make_token(char *e)
{
    int        position = 0;
    int        i;
    regmatch_t pmatch;

    /* 表示已经被识别出来的token数量 */
    nr_token = 0;

    while (e[position] != '\0') {
        /* Try all rules one by one. */
        for (i = 0; i < NR_REGEX; i++) {
            if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 &&
                pmatch.rm_so == 0) {
                char *substr_start = e + position;
                int   substr_len   = pmatch.rm_eo;

                Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
                    i, rules[i].regex, position, substr_len, substr_len,
                    substr_start);
                /* 每个token都有一定长度需要记录当前token有多长 */
                position += substr_len;

                /**
                 * TODO: Now a new token is recognized with rules[i]. Add codes
                 * to record the token in the array `tokens'. For certain types
                 * of tokens, some extra actions should be performed.
                 */
                /**
                 * TODO：现在一个新的 token 被 rules[i] 识别。 
                 * 添加代码以将tookens记录在数组“tookens”中。 对于某些类型的tookens，应该执行一些额外的操作。
                 * 对于十六进制的token，还需要原样保存，因为它们还需要经过一次转换才能进行运算
                 */

                switch (rules[i].token_type) {
                case TK_HEX:
                    sprintf(tokens[nr_token].str, "%.*s", substr_len,
                            substr_start);
                    debug_log("%s\n", substr_start);
                    debug_log("%s\n", tokens[nr_token].str);
                default:
                    tokens[nr_token].type = rules[i].token_type;
                    nr_token++;
                }
                break;
            }
        }

        if (i == NR_REGEX) {
            printf("no match at position %d\n%s\n%*.s^\n", position, e,
                   position, "");
            return false;
        }
    }

    return true;
}

/**
 * 用于判断表达式是否被一对匹配的括号包围着, 同时检查表达式的左右括号是否匹配, 
 * 如果不匹配, 这个表达式肯定是不符合语法的, 也就不需要继续进行求值了
 */
// static word_t check_parentheses(int p, int q)
// {
//     return 0;
// }

static word_t eval(int p, int q, bool *success)
{
    // if (p > q) {
    //     /* Bad expression */
    // } else if (p == q) {
    //     /**
    //      * 单个token，只有一个字符
    //      * 目前来说应该就是个数字，直接返回数字的值即可
    //      */
    //     // single token
    //     word_t val;
    //     switch (tokens[p].type) {
    //     case TK_NUM:
    //         val = strtoul(tokens[s].str, NULL, 0);
    //         break;
    //     default:
    //         assert(0);
    //     }
    // } else if (check_parentheses(p, q) == true) {
    //     /**
    //      * The expression is surrounded by a matched pair of parentheses.
    //      * If that is the case, just throw away the parentheses.
    //      */
    //     return eval(p + 1, q - 1);
    // } else {
    //     op   = the position of 主运算符 in the token expression;
    //     val1 = eval(p, op - 1);
    //     val2 = eval(op + 1, q);

    //     switch (op_type) {
    //     case '+':
    //         return val1 + val2;
    //     case '-': /* ... */
    //     case '*': /* ... */
    //     case '/': /* ... */
    //     default:
    //         assert(0);
    //     }
    // }
    return 0;
}

word_t expr(char *e, bool *success)
{
    if (!make_token(e)) {
        *success = false;
        return 0;
    }
    eval(1,2,success);
    *success = true;

    /* TODO: Insert codes to evaluate the expression. */
    return strtoul(tokens[0].str, NULL, 0);
}

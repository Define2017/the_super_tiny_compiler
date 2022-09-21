/**
 * The super tiny compiler （rewrite by in c）
 * 原项目地址: https://github.com/jamiebuilds/the-super-tiny-compiler
 * 中文翻译地址：https://github.com/521xueweihan/OneFile/blob/main/src/javascript/the-super-tiny-compiler.js
 *
 * 编译器的简单概念：将源语言翻译成目标语言。
 * 编译过程通常由词法分析器（lexer）、语法分析器（parser）、代码生成器（generater）三部分组成。
 * 词法分析器：将源代码序列化为Token数据结构对象。
 * 语法分析器：将Token数据对象进一步序列化为抽象语法树（AST）数据结构对象。
 * 代码生成器：将抽象语法树数据对象序列化为目标代码。
 * 一分钟图文并茂的编译器原理介绍：https://www.bilibili.com/video/BV1Tc411h7rj?is_story_h5=false&p=1&share_from=ugc&share_medium=ipad&share_plat=ios&share_source=WEIXIN&share_tag=s_i&timestamp=1663335535&unique_k=ztfqvLf&vd_source=06969d50da2a83309a5fc7fd4dcc4d73
 *
 * 项目的目标就是简化编译器的概念，并实现一个将类似 LISP 函数调用的源语言编译成类似 C 函数调用的目标语言的编译器。
 * 实现的编译效果如下：
 * 源语言 LISP   =======>    目标语言 C
 * (add 2 2)                 add(2, 2)
 * (subtract 4 2)            subtract(4, 2)
 * (add 2 (subtract 4 2))    add(2, subtract(4, 2))
 *
 * 以(add 2 (subtract 4 2))的编译过程为例，详细编译过程如下：
 * 1、词法分析器过程：
 * LISP       ===========>   Token
 * (add 2 (subtract 4 2))    [
 *                              { type: 'Parent', value: '('        },
 *                              { type: 'Name',   value: 'add'      },
 *                              { type: 'Number', value: '2'        },
 *                              { type: 'Parent', value: '('        },
 *                              { type: 'Name',   value: 'subtract' },
 *                              { type: 'Number', value: '4'        },
 *                              { type: 'Number', value: '2'        },
 *                              { type: 'Parent', value: ')'        },
 *                              { type: 'Parent', value: ')'        },
 *                           ]
 *
 * 算法：就是逐字符的扫描源码，将一组满足设定Token结构的字符放入Token数据对象中。这里将左右括号分别识别为一个Parent类型的token，
 * 将add 、subtract识别为Name类型的token，将数字识别为Number类型的token，虽然例子没有涉及到字符和字符串，但是代码实现上也有考虑，
 * 将字符、字符串分别识别为Char、Str类型的token。这个规则不是唯一的，可以自己定义Token的结构，然后实现代码。这里Token类型的种类就
 * 是对源语言的关键字、标识符、特殊字符、字面量值作分类。
 * --------------------------------------------------------------------------------------------------------------------------*
 *
 * 2、语法分析器过程：
 * Token          ========================>      AST
 * [                                             {
 *    { type: 'Parent', value: '('        },       type: 'Program',
 *    { type: 'Name',   value: 'add'      },           body: [{
 *    { type: 'Number', value: '2'        },             type: 'CallExpression',
 *    { type: 'Parent', value: '('        },             name: 'add',
 *    { type: 'Name',   value: 'subtract' },             params: [{
 *    { type: 'Number', value: '4'        },               type: 'NumberLiteral',
 *    { type: 'Number', value: '2'        },               value: '2',
 *    { type: 'Parent', value: ')'        },               }, {
 *    { type: 'Parent', value: ')'        },               type: 'CallExpression',
 * ]                                                       name: 'subtract',
 *                                                         params: [{
 *                                                           type: 'NumberLiteral',
 *                                                           value: '4',
 *                                                           }, {
 *                                                           type: 'NumberLiteral',
 *                                                           value: '2',
 *                                                         }]
 *                                                       }]
 *                                                     }]
 *                                               }
 *
 * 算法：首先增加了一个类型为Program的父节点用来表示程序，程序的主体部分是从Token数据对象中序列化。对Token中类型为Parent的一组左右
 * 括号识别为表达式，表达式由类型、名称、参数表组成。参数有两种类型一种是字面量（数值、字符），另一种是表达式。由于表达式中会嵌套表
 * 达式作为参数，所以实现时使用了递归方式来解析表达式。递归是常用的方式实现和理解上比较清晰、简单、容易，算法能力强可以考虑循环。
 * --------------------------------------------------------------------------------------------------------------------------*
 *
 * 3、代码生成器过程：
 * AST         ========================>    destinate code
 * {                                        add(2, subtract(4, 2))\n
 *   type: 'Program',
 *       body: [{
 *         type: 'CallExpression',
 *         name: 'add',
 *         params: [{
 *           type: 'NumberLiteral',
 *           value: '2',
 *           }, {
 *           type: 'CallExpression',
 *           name: 'subtract',
 *           params: [{
 *             type: 'NumberLiteral',
 *             value: '4',
 *             }, {
 *             type: 'NumberLiteral',
 *             value: '2',
 *           }]
 *         }]
 *       }]
 * }
 *
 * 算法：代码生成过程与语法解析过程相似，根据抽象语法树的内容逐步生成目标代码字符串，程序主体中多个表达式之间用换行符\n分割。表达式
 * 参数之间用逗号分割。由于表达式中会嵌套表达式，所以代码生成器同语法分析器一样使用了递归
 * 来生成表达式。
 * --------------------------------------------------------------------------------------------------------------------------*
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Totken
typedef enum TokenType
{
  Parent,
  Name,
  Number,
  Char,
  Str,
} TokenType;
char *tokenType[] = {"Parent", "Name", "Number", "Char", "Str"};
typedef struct Token
{
  TokenType type;
  char *value;
  struct Token *next;
} Token;
// AST
typedef enum ASTType
{
  Program,
  NumberLiteral,
  StringLiteral,
  CallExpression,
} ASTType;
char *astType[] = {"Program", "NumberLiteral", "StringLiteral", "CallExpression"};
// 参数
typedef struct Param
{
  ASTType type;
  union
  {
    char *literal;
    struct Express *express;
  } value; 
  struct Param *next;
} Param;
// 表达式
typedef struct Express
{
  char *name;
  struct Param *params;
  struct Express *next;
} Express;
// AST 抽象语法树
typedef struct AST
{
  ASTType type;
  struct Express *body;
} AST;
// lexer 词法分析器
const Token *lexer(const char *src)
{
  Token *const tokens = (Token *const)malloc(sizeof(Token));
  memset(tokens, 0, sizeof(Token));
  tokens->next = NULL;

  int current = 0;
  Token *token = (Token *)tokens;
  while (src[current] != 0)
  {
    // 空白字符
    if (src[current] == ' ' || src[current] == '\n' || src[current] == '\t')
    {
      current++;
      continue;
    }

    token->next = (Token *)malloc(sizeof(Token));
    token = token->next;
    token->next = NULL;
    // 括号处理
    if (src[current] == '(' || src[current] == ')')
    {
      token->type = Parent;
      token->value = (char *)malloc(sizeof(char) * 2);
      token->value[0] = src[current];
      token->value[1] = 0;
      current++;
      continue;
    }
    // 数字字面量
    if ('0' <= src[current] && src[current] <= '9')
    {
      token->type = Number;
      int num_count = 0;
      while ('0' <= src[current] && src[current] <= '9')
      {
        num_count++;
        current++;
      }
      current -= num_count;
      token->value = (char *)malloc(sizeof(char) * (num_count + 1));
      for (int i = 0; i < num_count; i++)
      {
        token->value[i] = src[current];
        current++;
      }
      token->value[num_count] = 0;
      continue;
    }
    // 字符字面量
    if (src[current] == '\'' && src[current - 1] != '\\')
    {
      token->type = Char;
      current++;
      int num_count = 0;
      while (!(src[current] == '\'' && (src[current - 1] != '\\' || (src[current - 1] == '\\' && src[current - 2] == '\\'))))
      {
        num_count++;
        current++;
      }
      current -= num_count;
      token->value = (char *)malloc(sizeof(char) * (num_count + 1));
      for (int i = 0; i < num_count; i++)
      {
        token->value[i] = src[current];
        current++;
      }
      token->value[num_count] = 0;
      current++;
      continue;
    }

    if (src[current] == '\"' && src[current - 1] != '\\')
    {
      token->type = Str;
      current++;
      int num_count = 0;
      while (!(src[current] == '\"' && (src[current - 1] != '\\' || (src[current - 1] == '\\' && src[current - 2] == '\\'))))
      {
        num_count++;
        current++;
      }
      current -= num_count;
      token->value = (char *)malloc(sizeof(char) * (num_count + 1));
      for (int i = 0; i < num_count; i++)
      {
        token->value[i] = src[current];
        current++;
      }
      token->value[num_count] = 0;
      current++;
      continue;
    }
    // 标识符
    if (('a' <= src[current] && 'z' >= src[current]) || ('A' <= src[current] && 'Z' >= src[current]) || src[current] == '_')
    {
      token->type = Name;
      int num_count = 0;
      while (('a' <= src[current] && src[current] <= 'z') || ('A' <= src[current] && src[current] <= 'Z') || src[current] == '_' || ('0' <= src[current] && src[current] <= '9'))
      {
        num_count++;
        current++;
      }
      current -= num_count;
      token->value = (char *)malloc(sizeof(char) * (num_count + 1));
      for (int i = 0; i < num_count; i++)
      {
        token->value[i] = src[current];
        current++;
      }
      token->value[num_count] = '\0';
      continue;
    }

    if (1)
    {
      printf("unknow token");
      break;
    }
  }
  return tokens;
}
// paser 语法分析器
Express *parser_express(const Token **token_ptr)
{
  const Token *token = *token_ptr;
  Express *express = (Express *)malloc(sizeof(Express));
  express->next = NULL;
  express->name = token->value;
  Param *param = NULL;
  param = express->params = (Param *)malloc(sizeof(Param));
  // 解析表达式的参数
  while (token->next && !(token->next->type == Parent && token->next->value[0] == ')'))
  {
    token = token->next;
    param->next = (Param *)malloc(sizeof(Param));
    param = param->next;
    param->next = NULL;
    if (token->type == Parent && token->value[0] == '(')
    {
      token = token->next;
      param->type = CallExpression;
      param->value.express = parser_express(&token);
      continue;
    }

    if (token->type == Number)
    {
      param->type = NumberLiteral;
      param->value.literal = token->value;
      continue;
    }
    if (token->type == Char || token->type == Str)
    {
      param->type = StringLiteral;
      param->value.literal = token->value;
      continue;
    }
  }

  if (token->next->type == Parent && token->next->value[0] == ')')
  {
    token = token->next;
  }
  *token_ptr = token;
  return express;
}

const AST *parser(const Token *tokens)
{
  AST *const ast = (AST *const)malloc(sizeof(AST));
  memset(ast, 0, sizeof(AST));
  ast->type = Program;
  Express *body = ast->body = (Express *)malloc(sizeof(Express));
  const Token *token = (const Token *)tokens;

  while (token->next)
  {
    token = token->next;
    if (token->type == Parent && token->value[0] == '(')
    {
      token = token->next;
      body->next = parser_express(&token);
      body = body->next;
    }
  }
  return ast;
}

// code generater 代码生成器
char *generate_express(const Express *express)
{
  char *out_exp = (char *)malloc(sizeof(char) * (strlen(express->name) + 3));
  memset(out_exp, 0, sizeof(char));
  strcpy(out_exp, express->name);
  strcat(out_exp, "(");
  Param *param = express->params;
  int param_count = 0;
  while (param->next)
  {
    param_count++;
    param = param->next;
    if (param->type == CallExpression)
    {
      char *sub_expr = generate_express(param->value.express);
      out_exp = (char *)realloc(out_exp, sizeof(char) * (strlen(out_exp) + strlen(sub_expr) + 1 + (param_count > 1 ? 1 : 0)));
      if (param_count > 1)
      {
        strcat(out_exp, ",");
      }
      strcat(out_exp, sub_expr);
      continue;
    }

    if (param->type == NumberLiteral || param->type == StringLiteral)
    {
      out_exp = (char *)realloc(out_exp, sizeof(char) * (strlen(out_exp) + strlen(param->value.literal) + 3 + (param_count > 1 ? 1 : 0)));
      if (param_count > 1)
      {
        strcat(out_exp, ",");
      }
      if (param->type == StringLiteral)
      {
        strcat(out_exp, "\"");
      }
      strcat(out_exp, param->value.literal);
      if (param->type == StringLiteral)
      {
        strcat(out_exp, "\"");
      }
      continue;
    }
  }

  out_exp = (char *)realloc(out_exp, sizeof(char) * (strlen(out_exp) + strlen(")") + 1));
  strcat(out_exp, ")");
  return out_exp;
}

const char *codeGenerator(const AST *ast)
{
  int exp_count = 0;
  char *out = (char *)malloc(12);
  strcpy(out, "Code is:\n");
  Express *body = ast->body;
  while (body->next)
  {
    exp_count++;
    body = body->next;
    char *expr = generate_express(body);
    out = (char *)realloc(out, sizeof(char) * (strlen(out) + strlen(expr) + 2 + 2 * exp_count));
    strcat(out, "  ");
    strcat(out, expr);
    strcat(out, "\n");
  }
  return out;
}
// free tokens
void free_token(Token *tokens)
{
  while (tokens)
  {
    Token *token = tokens;
    tokens = tokens->next;
    free(token->value);
    free(token);
  }
}
// free AST
void free_express(Express *express)
{
  free(express->name);
  while (express->params)
  {
    Param *param = express->params;
    express->params = express->params->next;
    if (param->type == NumberLiteral || param->type == StringLiteral)
    {
      free(param->value.literal);
    }
    if (param->type == CallExpression)
    {
      free_express(param->value.express);
    }
    free(param);
  }
  free(express);
}

void free_AST(AST *ast)
{
  while (ast->body)
  {
    Express *express = ast->body;
    ast->body = ast->body->next;
    free_express(express);
  }
  free(ast);
}
// 打印tokens
void print_tokens(const Token *token)
{
  printf("\nToken is:\n");
  while (token->next)
  {
    token = token->next;
    printf("  type = %-6s  value = %s\n", tokenType[token->type], token->value);
  }
}

// 打印AST
void print_space_text(size_t space_count)
{
  while (space_count)
  {
    space_count--;
    printf("%-s", "  ");
  }
}

void print_express(const Express *express, size_t space_count)
{
  const Param *param = NULL;
  print_space_text(space_count);
  printf("type = %s  name = %s\n", astType[CallExpression], express->name);
  param = express->params;
  while (param->next)
  {
    param = param->next;
    if (param->type == NumberLiteral)
    {
      print_space_text(space_count);
      printf("  type = %s  value = %s\n", astType[NumberLiteral], param->value.literal);
      continue;
    }
    if (param->type == StringLiteral)
    {
      print_space_text(space_count);
      printf("  type = %s  value = %s\n", astType[StringLiteral], param->value.literal);
      continue;
    }
    if (param->type == CallExpression)
    {
      print_express(param->value.express, ++space_count);
      continue;
    }
  }
}

void print_ast(const AST *const ast)
{
  printf("\nAST is:\n");
  const Express *body = (const Express *)ast->body;
  while (body->next)
  {
    body = body->next;
    print_express(body, 1);
  }
}

int main(int argc, char *argv[])
{
  const Token *tokens = lexer("(add 2 (subtract 4 2))\n(strcat \'H\' (strcat \"ello\" \"world\"))");
  print_tokens(tokens);
  const AST *ast = parser(tokens);
  print_ast(ast);
  const char *code = codeGenerator(ast);
  printf("\n%s", code);
  free((Token *)tokens);
  free((AST *)ast);
  free((char *)code);
  return 0;
}
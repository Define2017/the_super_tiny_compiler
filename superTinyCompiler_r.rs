/// The super tiny compiler （rewrite by in Rust）
/// 原项目地址: https://github.com/jamiebuilds/the-super-tiny-compiler
/// 中文翻译地址：https://github.com/521xueweihan/OneFile/blob/main/src/javascript/the-super-tiny-compiler.js
///
/// 编译器的简单概念：将源语言翻译成目标语言。
/// 编译过程通常由词法分析器（lexer）、语法分析器（parser）、代码生成器（generater）三部分组成。
/// 词法分析器：将源代码序列化为Token数据结构对象。
/// 语法分析器：将Token数据对象进一步序列化为抽象语法树（AST）数据结构对象。
/// 代码生成器：将抽象语法树数据对象序列化为目标代码。
/// 一分钟图文并茂的编译器原理介绍：https://www.bilibili.com/video/BV1Tc411h7rj?is_story_h5=false&p=1&share_from=ugc&share_medium=ipad&share_plat=ios&share_source=WEIXIN&share_tag=s_i&timestamp=1663335535&unique_k=ztfqvLf&vd_source=06969d50da2a83309a5fc7fd4dcc4d73
///
/// 项目的目标就是简化编译器的概念，并实现一个将类似 LISP 函数调用的源语言编译成类似 C 函数调用的目标语言的编译器。
/// 实现的编译效果如下：
/// 源语言 LISP   =======>    目标语言 C
/// (add 2 2)                 add(2, 2)
/// (subtract 4 2)            subtract(4, 2)
/// (add 2 (subtract 4 2))    add(2, subtract(4, 2))
///
/// 以(add 2 (subtract 4 2))的编译过程为例，详细编译过程如下：
/// 1、词法分析器过程：
/// LISP       ===========>   Token
/// (add 2 (subtract 4 2))    [
///                              { type: 'Parent', value: '('        },
///                              { type: 'Name',   value: 'add'      },
///                              { type: 'Number', value: '2'        },
///                              { type: 'Parent', value: '('        },
///                              { type: 'Name',   value: 'subtract' },
///                              { type: 'Number', value: '4'        },
///                              { type: 'Number', value: '2'        },
///                              { type: 'Parent', value: ')'        },
///                              { type: 'Parent', value: ')'        },
///                           ]
///
/// 算法：就是逐字符的扫描源码，将一组满足设定Token结构的字符放入Token数据对象中。这里将左右括号分别识别为一个Parent类型的token，
/// 将add 、subtract识别为Name类型的token，将数字识别为Number类型的token，虽然例子没有涉及到字符和字符串，但是代码实现上也有考虑，
/// 将字符、字符串分别识别为Char、Str类型的token。这个规则不是唯一的，可以自己定义Token的结构，然后实现代码。这里Token类型的种类就
/// 是对源语言的关键字、标识符、特殊字符、字面量值作分类。
/// --------------------------------------------------------------------------------------------------------------------------*
///
/// 2、语法分析器过程：
/// Token          ========================>      AST
/// [                                             {
///    { type: 'Parent', value: '('        },       type: 'Program',
///    { type: 'Name',   value: 'add'      },           body: [{
///    { type: 'Number', value: '2'        },             type: 'CallExpression',
///    { type: 'Parent', value: '('        },             name: 'add',
///    { type: 'Name',   value: 'subtract' },             params: [{
///    { type: 'Number', value: '4'        },               type: 'NumberLiteral',
///    { type: 'Number', value: '2'        },               value: '2',
///    { type: 'Parent', value: ')'        },               }, {
///    { type: 'Parent', value: ')'        },               type: 'CallExpression',
/// ]                                                       name: 'subtract',
///                                                         params: [{
///                                                           type: 'NumberLiteral',
///                                                           value: '4',
///                                                           }, {
///                                                           type: 'NumberLiteral',
///                                                           value: '2',
///                                                         }]
///                                                       }]
///                                                     }]
///                                               }
///
/// 算法：首先增加了一个类型为Program的父节点用来表示程序，程序的主体部分是从Token数据对象中序列化。对Token中类型为Parent的一组左右
/// 括号识别为表达式，表达式由类型、名称、参数表组成。参数有两种类型一种是字面量（数值、字符），另一种是表达式。由于表达式中会嵌套表
/// 达式作为参数，所以实现时使用了递归方式来解析表达式。递归是常用的方式实现和理解上比较清晰、简单、容易，算法能力强可以考虑循环。
/// --------------------------------------------------------------------------------------------------------------------------*
///
/// 3、代码生成器过程：
/// AST         ========================>    destinate code
/// {                                        add(2, subtract(4, 2))\n
///   type: 'Program',
///       body: [{
///         type: 'CallExpression',
///         name: 'add',
///         params: [{
///           type: 'NumberLiteral',
///           value: '2',
///           }, {
///           type: 'CallExpression',
///           name: 'subtract',
///           params: [{
///             type: 'NumberLiteral',
///             value: '4',
///             }, {
///             type: 'NumberLiteral',
///             value: '2',
///           }]
///         }]
///       }]
/// }
///
/// 算法：代码生成过程与语法解析过程相似，根据抽象语法树的内容逐步生成目标代码字符串，程序主体中多个表达式之间用换行符\n分割。表达式
/// 参数之间用逗号分割。由于表达式中会嵌套表达式，所以代码生成器同语法分析器一样使用了递归
/// 来生成表达式。
/// --------------------------------------------------------------------------------------------------------------------------*

// Totken
enum Token {
    Parent(String),
    Name(String),
    Number(String),
    Char(String),
    Str(String),
}
// AST
enum ASTType {
    Program,
    NumberLiteral(String),
    StringLiteral(String),
    CallExpression(Expr),
}
struct Expr {
    name: String,
    params: Vec<ASTType>,
}
// AST 抽象语法树
struct AST {
    categary: ASTType,
    body: Vec<Expr>,
}
// lexer 词法分析器
fn lexer(srcs: String) -> Vec<Token> {
    let mut tokens = Vec::<Token>::new();
    let mut src_cur: usize = 0;
    let src: Vec<char> = srcs.chars().collect::<Vec<_>>();
    while src.len() > src_cur && src[src_cur] != '\0' {
        // 空白字符
        if src[src_cur] == ' ' || src[src_cur] == '\n' || src[src_cur] == '\t' {
            src_cur += 1;
            continue;
        }
        // 括号处理tokens_cur
        if src[src_cur] == '(' || src[src_cur] == ')' {
            tokens.push(Token::Parent(src[src_cur].to_string()));
            src_cur += 1;
            continue;
        }
        // 数字字面量
        if '0' <= src[src_cur] && src[src_cur] <= '9' {
            let mut num_literal: String = String::from(src[src_cur].to_string());
            src_cur += 1;
            while '0' <= src[src_cur] && src[src_cur] <= '9' {
                num_literal.push(src[src_cur]);
                src_cur += 1;
            }
            tokens.push(Token::Number(num_literal));
            continue;
        }
        // 字符字面量
        if src[src_cur] == '\'' && src[src_cur - 1] != '\\' {
            let mut char_literal: String = String::from("");
            src_cur += 1;
            while !(src[src_cur] == '\''
                && (src[src_cur - 1] != '\\'
                    || (src[src_cur - 1] == '\\' && src[src_cur - 2] == '\\')))
            {
                char_literal.push(src[src_cur]);
                src_cur += 1;
            }
            tokens.push(Token::Char(char_literal));
            src_cur += 1;
            continue;
        }

        if src[src_cur] == '\"' && src[src_cur - 1] != '\\' {
            let mut str_literal: String = String::from("");
            src_cur += 1;
            while !(src[src_cur] == '\"'
                && (src[src_cur - 1] != '\\'
                    || (src[src_cur - 1] == '\\' && src[src_cur - 2] == '\\')))
            {
                str_literal.push(src[src_cur]);
                src_cur += 1;
            }
            tokens.push(Token::Str(str_literal));
            src_cur += 1;
            continue;
        }
        // 标识符
        if ('a' <= src[src_cur] && src[src_cur] <= 'z')
            || ('A' <= src[src_cur] && src[src_cur] <= 'Z')
            || src[src_cur] == '_'
        {
            let mut ide_literal: String = String::from(src[src_cur]);
            src_cur += 1;
            while ('a' <= src[src_cur] && src[src_cur] <= 'z')
                || ('A' <= src[src_cur] && src[src_cur] <= 'Z')
                || src[src_cur] == '_'
                || ('0' <= src[src_cur] && src[src_cur] <= '9')
            {
                ide_literal.push(src[src_cur]);
                src_cur += 1;
            }
            tokens.push(Token::Name(ide_literal));
            continue;
        }

        if true {
            println!("unknow token");
            break;
        }
    }
    return tokens;
}
// paser 语法分析器
fn parser_express(tokens: &Vec<Token>, index_t: &mut usize) -> Expr {
    let mut index: usize = *index_t;
    index += 1;
    let mut express = Expr {
        name: match &tokens[index] {
            Token::Name(value) => value.to_string(),
            _other => "".to_string(),
        },
        params: Vec::<ASTType>::new(),
    };
    // 解析表达式的参数
    index += 1; //tokens[index].type == .Parent && tokens.items[index].value[0] == ')'
    while tokens.len() > index {
        match &tokens[index] {
            Token::Parent(value) => {
                if value == ")" {
                    break;
                }
                if value == "(" {
                    express
                        .params
                        .push(ASTType::CallExpression(parser_express(&tokens, &mut index)));
                }
                index += 1;
                continue;
            }
            Token::Number(value) => {
                express
                    .params
                    .push(ASTType::NumberLiteral(value.to_string()));
                index += 1;
                continue;
            }
            Token::Char(value) | Token::Str(value) => {
                express
                    .params
                    .push(ASTType::StringLiteral(value.to_string()));
                index += 1;
                continue;
            }
            _other => index += 1,
        }
    }
    *index_t = index;
    return express;
}

fn parser(tokens: Vec<Token>) -> AST {
    let mut ast = AST {
        categary: ASTType::Program,
        body: Vec::<Expr>::new(),
    };
    let mut token_cur: usize = 0;
    loop {
        if tokens.len() <= token_cur {
            break;
        }
        match &tokens[token_cur] {
            Token::Parent(value) => {
                if value == "(" {
                    ast.body.push(parser_express(&tokens, &mut token_cur));
                    token_cur += 1;
                    continue;
                }
            }
            _other => {
                break;
            }
        }
    }
    return ast;
}

// code generater 代码生成器
fn generate_express(express: &Expr) -> String {
    let mut out_exp = String::from("");
    out_exp.push_str(&express.name[..]);
    out_exp.push('(');
    let mut param_count: u64 = 0;
    for param in express.params.iter() {
        param_count += 1;
        if param_count > 1 {
            out_exp.push(',');
        }
        match param {
            ASTType::NumberLiteral(value) => {
                out_exp.push_str(value);
                continue;
            }
            ASTType::StringLiteral(value) => {
                out_exp.push('\"');
                out_exp.push_str(value);
                out_exp.push('\"');
                continue;
            }
            ASTType::CallExpression(value) => {
                out_exp.push_str(&generate_express(&value)[..]);
                continue;
            }
            _other => continue,
        }
    }
    out_exp.push(')');
    return out_exp;
}

fn code_generator(ast: AST) -> String {
    let mut out = String::from("Code is:\n");
    for body in ast.body.iter() {
        out.push_str("  ");
        out.push_str(&generate_express(&body)[..]);
        out.push('\n');
    }
    return out;
}

// 打印tokens
fn print_tokens(tokens: &Vec<Token>) {
    print!("\nToken is:\n");
    for token in tokens.iter() {
        match token {
            Token::Parent(value) => print!("  type = {}  value = {}\n", "Parent", value),
            Token::Name(value) => print!("  type = {}  value = {}\n", "Name", value),
            Token::Number(value) => print!("  type = {}  value = {}\n", "Number", value),
            Token::Char(value) => print!("  type = {}  value = {}\n", "Char", value),
            Token::Str(value) => print!("  type = {}  value = {}\n", "Str", value),
        }
    }
}
// 打印AST
fn print_space_text(space_count: u64) {
    let mut space_count_t = space_count;
    while space_count_t > 0 {
        space_count_t -= 1;
        print!("  ");
    }
}

fn print_express(express: &Expr, space_count: u64) {
    print_space_text(space_count);
    print!("type = {}  name = {}\n", "CallExpression", express.name);
    let mut space_count_t = space_count;
    for param in express.params.iter() {
        match param {
            ASTType::NumberLiteral(value) => {
                print_space_text(space_count);
                print!("  type = {}  value = {}\n", "NumberLiteral", value);
                continue;
            }
            ASTType::StringLiteral(value) => {
                print_space_text(space_count);
                print!("  type = {}  value = {}\n", "StringLiteral", value);
                continue;
            }
            ASTType::CallExpression(value) => {
                space_count_t += 1;
                print_express(&value, space_count_t);
                continue;
            }
            _other => break,
        }
    }
}

fn print_ast(ast: &AST) {
    print!("\nAST is:\n");
    for item in ast.body.iter() {
        print_express(&item, 1);
    }
}
fn main() {
    let tokens =
        lexer("(add 2 (subtract 4 2))\n(strcat \'H\' (strcat \"ello\" \"world\"))".to_string());
    print_tokens(&tokens);

    let ast = parser(tokens);
    print_ast(&ast);

    let codes = code_generator(ast);
    print!("{}", codes);
}

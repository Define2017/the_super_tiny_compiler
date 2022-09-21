//! The super tiny compiler （rewrite by in zig）
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
const std = @import("std");
const print = std.debug.print;
const Allocator = std.mem.Allocator;
const ArrayList = std.ArrayList;
// Totken
const TokenType = enum(u64) {
    Parent,
    Name,
    Number,
    Char,
    Str,
};
const Token = struct {
    type: TokenType,
    value: []const u8,
};
const Tokens = ArrayList(Token);
// AST
const ASTType = enum(u64) {
    Program,
    NumberLiteral,
    StringLiteral,
    CallExpression,
};
// 参数
const Param = struct {
    type: ASTType,
    value: union {
        literal: []const u8,
        express: Expr,
    }, // 这里为了结构上的统一使用联合体表示参数值，这样参数结构就只有类型、值、下一个指针三个固定的字段
};
const Params = ArrayList(Param);
// 表达式
const Expr = struct {
    name: []const u8,
    params: Params,
};
const Expres = ArrayList(Expr);
// AST 抽象语法树
const AST = struct {
    type: ASTType,
    body: Expres,
};
// lexer 词法分析器
fn lexer(allocator: Allocator, src: []const u8) !Tokens {
    var tokens: Tokens = Tokens.init(allocator);
    var src_cur: u64 = 0;
    while (src.len > src_cur and src[src_cur] != 0) {
        // 空白字符
        if (src[src_cur] == ' ' or src[src_cur] == '\n' or src[src_cur] == '\t') {
            src_cur += 1;
            continue;
        }
        // 括号处理tokens_cur
        if (src[src_cur] == '(' or src[src_cur] == ')') {
            try tokens.append(Token{
                .type = .Parent,
                .value = src[src_cur .. src_cur + 1],
            });
            src_cur += 1;
            continue;
        }
        // 数字字面量
        if ('0' <= src[src_cur] and src[src_cur] <= '9') {
            var num_count: u64 = 0;
            while (src.len > src_cur and ('0' <= src[src_cur] and src[src_cur] <= '9')) {
                num_count += 1;
                src_cur += 1;
            }
            try tokens.append(Token{
                .type = .Number,
                .value = src[src_cur - num_count .. src_cur],
            });
            continue;
        }
        // 字符字面量
        if (src[src_cur] == '\'' and src[src_cur - 1] != '\\') {
            src_cur += 1;
            var num_count: u64 = 0;
            while (src.len > src_cur and (!(src[src_cur] == '\'' and (src[src_cur - 1] != '\\' or (src[src_cur - 1] == '\\' and src[src_cur - 2] == '\\'))))) {
                num_count += 1;
                src_cur += 1;
            }
            try tokens.append(Token{
                .type = .Char,
                .value = src[src_cur - num_count .. src_cur],
            });
            src_cur += 1;
            continue;
        }

        if (src[src_cur] == '\"' and src[src_cur - 1] != '\\') {
            src_cur += 1;
            var num_count: u64 = 0;
            while (src.len > src_cur and (!(src[src_cur] == '\"' and (src[src_cur - 1] != '\\' or (src[src_cur - 1] == '\\' and src[src_cur - 2] == '\\'))))) {
                num_count += 1;
                src_cur += 1;
            }
            try tokens.append(Token{
                .type = .Str,
                .value = src[src_cur - num_count .. src_cur],
            });
            src_cur += 1;
            continue;
        }
        // 标识符
        if (('a' <= src[src_cur] and 'z' >= src[src_cur]) or ('A' <= src[src_cur] and 'Z' >= src[src_cur]) or src[src_cur] == '_') {
            var num_count: u64 = 0;
            while (src.len > src_cur and (('a' <= src[src_cur] and src[src_cur] <= 'z') or ('A' <= src[src_cur] and src[src_cur] <= 'Z') or src[src_cur] == '_' or ('0' <= src[src_cur] and src[src_cur] <= '9'))) {
                num_count += 1;
                src_cur += 1;
            }
            try tokens.append(Token{
                .type = .Name,
                .value = src[src_cur - num_count .. src_cur],
            });
            continue;
        }

        if (true) {
            print("unknow token\n", .{});
            break;
        }
    }
    return tokens;
}
// paser 语法分析器
fn parser_express(allocator: Allocator, tokens: Tokens, index_t: *u64) anyerror!Expr {
    var index: u64 = index_t.*;
    index += 1;
    var express = Expr{
        .name = tokens.items[index].value,
        .params = Params.init(allocator),
    };
    // 解析表达式的参数
    index += 1;
    while (tokens.items.len > index and !(tokens.items[index].type == .Parent and tokens.items[index].value[0] == ')')) {
        if (tokens.items[index].type == .Parent and tokens.items[index].value[0] == '(') {
            try express.params.append(Param{ .type = .CallExpression, .value = .{ .express = try parser_express(allocator, tokens, &index) } });
            index += 1;
            continue;
        }

        if (tokens.items[index].type == .Number) {
            try express.params.append(Param{ .type = .NumberLiteral, .value = .{ .literal = tokens.items[index].value } });
            index += 1;
            continue;
        }
        if (tokens.items[index].type == .Char or tokens.items[index].type == .Str) {
            try express.params.append(Param{ .type = .StringLiteral, .value = .{ .literal = tokens.items[index].value } });
            index += 1;
            continue;
        }
        index += 1;
    }
    index_t.* = index;
    return express;
}

fn parser(allocator: Allocator, tokens: Tokens) !AST {
    var ast = AST{
        .type = .Program,
        .body = Expres.init(allocator),
    };
    var index: u64 = 0;
    while (tokens.items.len > index and tokens.items[index].value[0] != 0) {
        if (tokens.items[index].type == .Parent and tokens.items[index].value[0] == '(') {
            try ast.body.append(try parser_express(allocator, tokens, &index));
            index += 1;
            continue;
        }
        index += 1;
    }
    return ast;
}

// code generater 代码生成器
fn generate_express(allocator: Allocator, express: Expr) anyerror!ArrayList([]const u8) {
    var out_exp = ArrayList([]const u8).init(allocator);
    try out_exp.append(express.name);
    try out_exp.append("(");
    var param_count: u64 = 0;
    for (express.params.items) |param| {
        param_count += 1;
        if (param_count > 1) {
            try out_exp.append(",");
        }
        if (param.type == .CallExpression) {
            var sub_expr = try generate_express(allocator, param.value.express);
            try out_exp.appendSlice(sub_expr.items[0..]);
            continue;
        }

        if (param.type == .NumberLiteral or param.type == .StringLiteral) {
            if (param.type == .StringLiteral) {
                try out_exp.append("\"");
            }
            try out_exp.append(param.value.literal);
            if (param.type == .StringLiteral) {
                try out_exp.append("\"");
            }
            continue;
        }
    }
    try out_exp.append(")");
    return out_exp;
}

fn codeGenerator(allocator: Allocator, ast: AST) !ArrayList([]const u8) {
    var out = ArrayList([]const u8).init(allocator);
    try out.append("Code is:\n");
    for (ast.body.items) |body| {
        try out.append("  ");
        var sub_expr = try generate_express(allocator, body);
        try out.appendSlice(sub_expr.items[0..]);
        try out.append("\n");
    }
    return out;
}

// 打印tokens
fn print_tokens(tokens: Tokens) void {
    print("\nToken is:\n", .{});
    for (tokens.items) |item| {
        print("  type = {s}  value = {s}\n", .{ item.type, item.value });
    }
}
// 打印AST
fn print_space_text(space_count: u64) void {
    var space_count_t = space_count;
    while (space_count_t > 0) {
        space_count_t -= 1;
        print("  ", .{});
    }
}

fn print_express(express: Expr, space_count: u64) void {
    print_space_text(space_count);
    print("type = {s}  name = {s}\n", .{ ASTType.CallExpression, express.name });
    var space_count_t = space_count;
    for (express.params.items) |param| {
        if (param.type == .NumberLiteral or param.type == .StringLiteral) {
            print_space_text(space_count);
            print("  type = {s}  value = {s}\n", .{ param.type, param.value.literal });
            continue;
        }
        if (param.type == .CallExpression) {
            space_count_t += 1;
            print_express(param.value.express, space_count_t);
            continue;
        }
    }
}

fn print_ast(ast: AST) void {
    print("\nAST is:\n", .{});
    for (ast.body.items) |item| {
        print_express(item, 1);
    }
}

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    const allocator = gpa.allocator();

    var tokens: Tokens = try lexer(allocator, "(add 2 (subtract 4 2))\n(strcat \'H\' (strcat \"ello\" \"world\"))");
    defer tokens.deinit();
    print_tokens(tokens);

    var ast = try parser(allocator, tokens);
    defer ast.body.deinit();
    print_ast(ast);

    var codes = try codeGenerator(allocator, ast);
    defer codes.deinit();
    for (codes.items) |code| {
        print("{s}", .{code});
    }
}

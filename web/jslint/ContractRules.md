# **智能合约语法说明**
Phantom 智能合约使用 JaveScript 语言编写，为了方便开发者更规范的，更安全的开发合约，在做合约语法检测时候，使用了 JSLint 做限制。[参考JSLint GitHub](./)。编辑合约时候，首先需要在 JSLint 里检测通过，才可以被 Phantom 系统检测为一个合法的合约。

JSLint 的标准语法在官方网站有详尽的说明。本文档的目的是作为一个完善文档，整理了原有的 JSLint 语法规则，同时对Phantom 修改后的规则进行了补充说明，文档会举例说明其用法。对于本文没有提到的部分，请参考 [JsLint 帮助手册](http://bumo.chinacloudapp.cn:36002/help.html)。

或者通过节点服务器、钱包地址访问文档 127.0.0.1:36002/jslint/help.html

## **检测工具**
   JSLint 检测工具地址：[JSLint 语法检测工具](http://bumo.chinacloudapp.cn:36002/jslint.html "JSLint 语法检测工具")

   或者通过节点服务器、钱包地址使用工具 127.0.0.1:36002/jslint/index.html

错误说明，在web工具里调试合约语法时候，会有详尽的错误描述。当输入如下代码时候

```javascript

"use strict";
function init(bar)
{
    
}
```

错误如下

```
Empty block.   2.0
{
```

错误原因：空的语句块，在第 2 行，第 0 列。

正确的代码如下：

```javascript

"use strict";
function init(bar)
{
    return;    
}
```

正确的检测结果，不会报出红色的 Warnings 信息

## **文本压缩**
合约文档写好之后，可以使用JSMin工具进行压缩，注意保存原文档，压缩是不可逆的操作。

[工具地址](../../../deploy/jsmin/)

## **Demo**
```javascript

"use strict";
function init(bar)
{
    /*init whatever you want*/
    return;
}

function main(input) 
{
    log(input);

    //for statement
    let i;
    for (i = 0; i < 5; i += 1) 
    {
        log(i);
    }

    //while statement
    let b = 10;
    while (b !== 0) 
    {
        b -= 1;
        log(b);
    }

    //if statement
    let compare = 1;
    if(compare === 1)
    {
        log("it is one");
    }
    else if(compare === 2)
    {
        log("it is two");
    }
    else
    {
        log("it is other");
    }

    //if statement
    if(compare !== 2)
    {
        log("no, different");
    }

    //switch statement
    let sw_value = 1;
    switch(sw_value)
    {
    case 1:
        log("switch 1");
        break;
    default:
        log("default");
    }

    //Number
    let my_num = Number(111);
    log(my_num);

    //String
    let my_str = String(111);
    log(my_str);

    //Boolean
    let my_bool = Boolean(111);
    log(my_bool);

    //Array
    let str_array = ["red","black"]; 
    log(str_array);

    //Array
    let num_array = [1,2,3,4];
    log(num_array);
}
```

## **规则列表**
 
- 严格检测声明;

   所有的源码在开始必须要添加 "use strict"; 字段。

- 语句块内尽量使用 'let' 声明变量

- 使用'===' 代替 '==' 判断比较；使用 '==' 代替 '！=' 比较

- 语句必须以 ';' 结束 

- 语句块必须用 '{}' 包括起来，且禁止空语句块。

- 'for' 的循环变量初始变量需在条件语句块之前声明，每次使用重新赋值.

- 禁用 '++' 和 '--'，使用 '+=' 和 '-=' 替代

- 禁止使用 'eval', 'void', 'this' 关键字

- 禁止使用 'new' 创建 'Number', 'String', 'Boolean'对象，可以使用其构造调用来获取对象

- 禁止使用的数组关键字创建数组

```javascript
"Array", "ArrayBuffer", "Float32Array", "Float64Array", 
"Int8Array", "Int16Array", "Int32Array", "Uint8Array", 
"Uint8ClampedArray", "Uint16Array", "Uint32Array"

let color = new Array(100); //编译报错

//可以使用替代 new Array(100) 语句;
let color = ["red","black"]; 
let arr = [1,2,3,4];
```

- 禁止使用的关键字
```javascript
"DataView", "decodeURI", "decodeURIComponent", "encodeURI", 
"encodeURIComponent", "Generator","GeneratorFunction", "Intl", 
"Promise", "Proxy", "Reflect", "System", "URIError", "WeakMap", 
"WeakSet", "Math", "Date"
```

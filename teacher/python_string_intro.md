# Python 字符串入门课

适合对象：刚学完数组/列表的 16 岁学生  
课程时长：60 分钟  
课程主题：Python 字符串基础

## 一、课程目标

这节课结束后，学生应该能做到：

1. 理解字符串是什么。
2. 会创建、访问、切片字符串。
3. 知道字符串和列表的相似点与不同点。
4. 会使用常见字符串方法。
5. 能完成简单的字符串处理题。

## 二、课前基础

假设学生已经学过列表，例如：

```python
arr = [1, 2, 3]

print(arr[0])
print(arr[1:3])

for x in arr:
    print(x)
```

本节课可以把字符串类比成“由字符组成的列表”。

## 三、课前预习题单（洛谷）

下面的题目只需要**已经学过的列表、循环、条件判断**就能完成，建议在上课前让学生独立做完。  
前 6 题是巩固旧知识，最后 2 题会提前碰到一点“字符”的概念，为本节课做铺垫（做不出来也没关系，上课会讲）。

### 巩固列表与循环

1. [P1046 [NOIP 2005 普及组] 陶陶摘苹果](https://www.luogu.com.cn/problem/P1046)  
   列表访问 + 条件计数。热身题，检查列表下标是否掌握。

2. [P1427 小鱼的数字游戏](https://www.luogu.com.cn/problem/P1427)  
   把一串数字**倒序输出**。为课上的 `s[::-1]` 埋伏笔——上完课可以回头问学生：字符串能不能也这样倒过来？

3. [P1428 小鱼比可爱](https://www.luogu.com.cn/problem/P1428)  
   双重循环 + 计数。和课上“遍历字符串统计字符”是同一个思维模式。

4. [P1567 统计天数](https://www.luogu.com.cn/problem/P1567)  
   遍历序列统计最长连续上升段。练“一边遍历一边维护计数器”。

5. [P5727 〖深基5.例3〗冰雹猜想](https://www.luogu.com.cn/problem/P5727)  
   循环模拟 + 把结果存进列表后倒序输出。

6. [P1047 [NOIP 2005 普及组] 校门外的树](https://www.luogu.com.cn/problem/P1047)  
   用列表做标记。巩固“列表可以修改”，正好和课上“字符串不可修改”形成对比。

### 提前感受字符（不要求会做）

7. [P5704 〖深基2.例6〗字母转换](https://www.luogu.com.cn/problem/P5704)  
   输入一个小写字母，输出对应的大写字母。就是课上 `upper()` 的最小版本。

8. [P5705 〖深基2.例7〗数字反转](https://www.luogu.com.cn/problem/P5705)  
   反转一个数。学生现在可能用算术方法做，上完课再告诉他：用字符串切片一行就能解决。

## 四、课堂安排

## 0-5 分钟：引入字符串

先提问：

> 如果列表可以存一组数字，那一句话在 Python 里怎么存？

示例：

```python
name = "Alice"
sentence = "I love Python"
```

解释：

字符串就是一串字符，比如：

```python
"hello"
"abc123"
"你好"
```

注意：

```python
"123"
```

这是字符串，不是数字。

## 5-15 分钟：字符串和列表很像

从学生熟悉的列表切入：

```python
s = "hello"
```

访问单个字符：

```python
print(s[0])  # h
print(s[1])  # e
print(s[4])  # o
```

倒数访问：

```python
print(s[-1])  # o
print(s[-2])  # l
```

长度：

```python
print(len(s))  # 5
```

对比列表：

```python
arr = [10, 20, 30]
print(arr[0])

s = "abc"
print(s[0])
```

结论：

字符串也可以用下标访问。

课堂小练习：

```python
word = "python"
```

让学生回答下面代码的结果：

```python
word[0]
word[2]
word[-1]
len(word)
```

参考答案：

```python
"p"
"t"
"n"
6
```

## 15-25 分钟：字符串切片

字符串切片和列表切片非常像。

```python
s = "python"
```

示例：

```python
print(s[0:2])   # py
print(s[2:5])   # tho
print(s[:3])    # pyt
print(s[3:])    # hon
print(s[::-1])  # nohtyp
```

重点：

```python
s[start:end]
```

包含 `start`，不包含 `end`。

课堂小练习：

```python
s = "programming"
```

让学生写代码取出：

```python
"pro"
"gram"
"ming"
```

参考答案：

```python
s = "programming"

print(s[:3])
print(s[3:7])
print(s[7:])
```

再让学生尝试倒序输出整个字符串：

```python
print(s[::-1])
```

## 25-35 分钟：字符串不能直接修改

这是字符串和列表的重要区别。

列表可以修改：

```python
arr = [1, 2, 3]
arr[0] = 100
print(arr)
```

字符串不能这样修改：

```python
s = "hello"
s[0] = "H"  # 报错
```

解释：

字符串是不可变的。  
如果想“修改”字符串，要创建一个新的字符串。

示例：

```python
s = "hello"
s = "H" + s[1:]
print(s)  # Hello
```

课堂小练习：

把：

```python
word = "cat"
```

改成：

```python
"bat"
```

参考答案：

```python
word = "cat"
word = "b" + word[1:]
print(word)
```

## 35-45 分钟：常见字符串操作

字符串拼接：

```python
a = "hello"
b = "world"

print(a + " " + b)
```

字符串重复：

```python
print("ha" * 3)  # hahaha
```

判断是否包含：

```python
s = "I love Python"

print("Python" in s)
print("Java" in s)
```

大小写转换：

```python
s = "hello"

print(s.upper())  # HELLO
print(s.lower())  # hello
```

去掉首尾空格：

```python
s = "  hello  "

print(s.strip())
```

查找：

```python
s = "banana"

print(s.find("na"))  # 2
```

替换：

```python
s = "I like Java"

print(s.replace("Java", "Python"))
```

分割字符串：

```python
s = "apple,banana,orange"
fruits = s.split(",")

print(fruits)
```

输出：

```python
['apple', 'banana', 'orange']
```

这里可以提醒学生：

`split()` 会把一个字符串变成一个列表。

## 45-55 分钟：综合练习

### 练习 1：统计字符个数

题目：

统计字符串里有多少个字母 `"a"`。

示例：

```python
s = "banana"
```

答案应该是：

```python
3
```

代码：

```python
s = "banana"
count = 0

for ch in s:
    if ch == "a":
        count += 1

print(count)
```

### 练习 2：判断回文字符串

回文字符串：正着读和倒着读一样。

例如：

```python
"level"
"madam"
```

代码：

```python
s = "level"

if s == s[::-1]:
    print("是回文")
else:
    print("不是回文")
```

### 练习 3：简单用户名检查

题目：

如果用户名长度小于 3，输出“太短”；否则输出“可以使用”。

```python
username = "tom"

if len(username) < 3:
    print("太短")
else:
    print("可以使用")
```

## 55-60 分钟：课堂总结

字符串可以像列表一样：

```python
s[0]
s[1:4]
len(s)
for ch in s:
    print(ch)
```

但字符串不能直接修改某个位置：

```python
s[0] = "A"  # 错误
```

常见字符串方法：

```python
upper()
lower()
strip()
find()
replace()
split()
```

一句话总结：

> 字符串就是一串字符。它和列表很像，可以访问、切片、遍历，但它不能直接修改。

## 五、洛谷课堂例题

下面 4 道题作为**课堂讲解例题**，配合上机演示使用。  
建议课上按顺序讲前两题，第三题视学生状态选讲，第四题只讲简化版思路。

### 1. P5733 自动修正

题目链接：[P5733 〖深基6.例1〗自动修正](https://www.luogu.com.cn/problem/P5733)

适合知识点：

1. 字符串输入。
2. `upper()` 方法。
3. 字符串遍历。

题意简述：

输入一个不包含空格的字符串，把里面的小写字母全部变成大写字母并输出。

推荐讲法：

先用最简单的 Python 写法：

```python
s = input()
print(s.upper())
```

再引导学生思考：

如果不用 `upper()`，能不能一个字符一个字符处理？

拓展版本：

```python
s = input()
ans = ""

for ch in s:
    if "a" <= ch <= "z":
        ans += chr(ord(ch) - 32)
    else:
        ans += ch

print(ans)
```

这一题适合放在课堂刚讲完 `upper()` 之后。

### 2. P5015 标题统计

题目链接：[P5015 [NOIP 2018 普及组] 标题统计](https://www.luogu.com.cn/problem/P5015)

适合知识点：

1. `input()` 读取一整行。
2. `for ch in s` 遍历字符串。
3. 统计符合条件的字符数量。

题意简述：

输入一行标题，统计其中非空格字符的数量。

推荐代码：

```python
s = input()
count = 0

for ch in s:
    if ch != " ":
        count += 1

print(count)
```

也可以用更短的写法：

```python
s = input()
print(len(s.replace(" ", "")))
```

教学建议：

第一种写法更适合新手，因为它能练习遍历和计数。  
第二种写法可以作为字符串方法的补充展示。

### 3. P1914 小书童——凯撒密码

题目链接：[P1914 小书童——凯撒密码](https://www.luogu.com.cn/problem/P1914)

适合知识点：

1. 字符串遍历。
2. 字符拼接。
3. `ord()` 和 `chr()`。
4. 取模 `%`。

题意简述：

给定一个移动位数 `n` 和一个只包含小写字母的字符串，把每个字母向后移动 `n` 位，超过 `z` 后从 `a` 继续。

推荐代码：

```python
n = int(input())
s = input()
ans = ""

for ch in s:
    x = ord(ch) - ord("a")
    x = (x + n) % 26
    ans += chr(x + ord("a"))

print(ans)
```

教学建议：

这题不建议放在第一次讲字符串的前半节课。  
可以在学生已经理解字符串遍历之后，用它引入“字符和数字之间可以互相转换”。

### 4. P1308 统计单词数

题目链接：[P1308 [NOIP 2011 普及组] 统计单词数](https://www.luogu.com.cn/problem/P1308)

适合知识点：

1. `lower()` 忽略大小写。
2. `split()` 切分单词。
3. 完整匹配单词。
4. 查找第一次出现的位置。

题意简述：

给定一个单词和一篇文章，统计这个单词在文章中出现了几次，并输出第一次出现的位置。匹配时不区分大小写，但必须是完整单词。

入门讲解版本：

如果暂时不要求输出第一次出现的位置，可以先做一个简化版：

```python
word = input().lower()
text = input().lower()

words = text.split()
count = 0

for w in words:
    if w == word:
        count += 1

print(count)
```

完整题目还需要输出第一次出现的位置，难度比前几题高。  
建议作为课后挑战题，而不是课堂主练习。

## 六、课后训练题单（洛谷）

上完课后让学生按档位顺序刷题。第一档要求全部完成；第二档至少完成 3 题；第三档是挑战题，能做几题做几题。  
每道题后面标注了主要考点，方便学生卡住时回看讲义对应部分。

### 第一档：基础巩固（对应课上核心内容）

1. [P5734 〖深基6.例6〗文字处理软件](https://www.luogu.com.cn/problem/P5734)  
   考点：拼接 `+`、切片、`find()`。相当于把课上所有基本操作串成一个模拟题，**最推荐的第一道课后题**。

2. [P1765 手机](https://www.luogu.com.cn/problem/P1765)  
   考点：字符串遍历 + 字符到按键次数的映射。练“对每个字符查表累加”。

3. [P1179 [NOIP 2010 普及组] 数字统计](https://www.luogu.com.cn/problem/P1179)  
   考点：把数字转成字符串后统计字符。可以用 `str(i).count("2")` 一行解决核心逻辑。

4. [P1125 [NOIP 2008 提高组] 笨小猴](https://www.luogu.com.cn/problem/P1125)  
   考点：统计每个字母出现次数，找最大/最小值。另外需要一个简单的质数判断，可提前给学生提示。

### 第二档：进阶练习（字符和数字互相转换）

5. [P1055 [NOIP 2008 普及组] ISBN 号码](https://www.luogu.com.cn/problem/P1055)  
   考点：遍历时跳过 `-`、`ord()`/`int()` 转换、校验码计算与替换。

6. [P1200 [USACO1.1] 你的飞碟在这儿](https://www.luogu.com.cn/problem/P1200)  
   考点：`ord(ch) - ord("A") + 1` 把字母变成数字，再求乘积取模。和课上凯撒密码的字符运算一脉相承。

7. [P1307 [NOIP 2011 普及组] 数字反转](https://www.luogu.com.cn/problem/P1307)  
   考点：切片倒序 + 处理负号和前导零。是预习题 P5705 的正式升级版。

8. [P1321 单词覆盖还原](https://www.luogu.com.cn/problem/P1321)  
   考点：枚举每个位置判断子串片段。练下标和切片的配合。

### 第三档：挑战题（拉开梯度）

9. [P1553 数字反转（升级版）](https://www.luogu.com.cn/problem/P1553)  
   考点：按 `.`、`/`、`%` 分类讨论后分段反转。细节多，练耐心和边界处理。

10. [P1781 宇宙总统](https://www.luogu.com.cn/problem/P1781)  
    考点：票数太大不能当数字比，先比长度再比字典序。体会“字符串也能比较大小”。

11. [P1598 垂直柱状图](https://www.luogu.com.cn/problem/P1598)  
    考点：字母频率统计 + 按行格式化输出。输出部分很考验思路清晰度。

12. [P1308 [NOIP 2011 普及组] 统计单词数（完整版）](https://www.luogu.com.cn/problem/P1308)  
    课上只讲了简化版（只统计次数），课后要求补上“第一次出现的位置”，做到完整 AC。

## 七、课后作业

1. 输入一个单词，输出它的倒序。
2. 输入一句话，统计里面有多少个空格。
3. 输入一个字符串，判断它是不是回文。
4. 输入一个邮箱，判断里面是否包含 `"@"`。
5. 把字符串 `"I like C++"` 改成 `"I like Python"`。
6. 独立重写课堂例题 P5733、P5015、P1914（不看讲义，自己再写一遍并提交 AC）。
7. 完成“课后训练题单”第一档的 4 道题（P5734、P1765、P1179、P1125）。
8. 从第二档中任选 3 题完成；学有余力再挑战第三档。

## 八、建议讲课节奏

讲课时不要一开始就大量介绍字符串方法。  
建议先让学生理解：

1. 字符串是什么。
2. 字符串可以用下标。
3. 字符串可以切片。
4. 字符串不能直接修改。

等这些概念稳了，再讲常用方法。  
这样学生更容易把字符串和刚学过的列表联系起来。

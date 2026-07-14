# Python 字符串入门课口语化讲稿

适合对象：刚学完数组/列表的 16 岁学生  
课程时长：约 60 分钟  
使用方式：老师可以按照下面的话术直接讲，也可以根据学生反应删减。

## 开场：0-5 分钟

同学，我们前面已经学过列表了。比如一个列表里面可以放很多数字：

```python
arr = [1, 2, 3]
```

那我现在问你一个问题：如果我想在 Python 里面存一句话，比如 `hello`，或者 `I love Python`，应该怎么存？

这时候我们就要用到今天的新内容：字符串。

字符串，简单说，就是一串字符。字符可以是字母、数字、符号，也可以是中文。

比如：

```python
name = "Alice"
sentence = "I love Python"
```

这里的 `"Alice"` 和 `"I love Python"` 都是字符串。

注意一个小细节：

```python
"123"
```

这个看起来像数字，但只要它被引号包起来，它就是字符串，不是整数。

你可以把字符串理解成：一排字符排成了一队。

## 字符串和列表很像：5-15 分钟

我们先看一个字符串：

```python
s = "hello"
```

你可以把它想成这样：

```text
h e l l o
0 1 2 3 4
```

每个字符都有自己的位置，这个位置我们叫下标，也叫索引。

所以我们可以这样取字符：

```python
s = "hello"

print(s[0])
print(s[1])
print(s[4])
```

你猜一下会输出什么？

答案是：

```python
h
e
o
```

这和列表很像。列表可以这样访问：

```python
arr = [10, 20, 30]
print(arr[0])
```

字符串也可以这样访问：

```python
s = "abc"
print(s[0])
```

所以你可以先记住一句话：字符串可以像列表一样用下标访问。

Python 里面还可以用负数下标。比如：

```python
s = "hello"

print(s[-1])
print(s[-2])
```

`-1` 表示最后一个字符，`-2` 表示倒数第二个字符。

所以输出是：

```python
o
l
```

如果我们想知道字符串有多长，可以用 `len()`：

```python
s = "hello"
print(len(s))
```

结果是 `5`，因为 `hello` 一共有 5 个字符。

现在你来试一下：

```python
word = "python"
```

请你说一说：

```python
word[0]
word[2]
word[-1]
len(word)
```

分别是什么？

答案是：

```python
"p"
"t"
"n"
6
```

## 字符串切片：15-25 分钟

刚才我们是一次取一个字符。那如果我想一次取一段呢？

这时候就用切片。

比如：

```python
s = "python"
```

我们可以这样写：

```python
print(s[0:2])
```

这表示从下标 `0` 开始，取到下标 `2` 前面。注意，不包含下标 `2`。

所以结果是：

```python
py
```

再看几个例子：

```python
s = "python"

print(s[0:2])
print(s[2:5])
print(s[:3])
print(s[3:])
```

输出是：

```python
py
tho
pyt
hon
```

这里有一个非常重要的规则：

```python
s[start:end]
```

包含 `start`，不包含 `end`。

也就是说，左边要，右边不要。

如果冒号前面不写，默认从开头开始：

```python
s[:3]
```

如果冒号后面不写，默认取到最后：

```python
s[3:]
```

还有一个很好用的写法：

```python
s[::-1]
```

它可以把字符串倒过来。

比如：

```python
s = "python"
print(s[::-1])
```

输出：

```python
nohtyp
```

现在我们练一下：

```python
s = "programming"
```

请你取出：

```python
"pro"
"gram"
"ming"
```

参考写法是：

```python
s = "programming"

print(s[:3])
print(s[3:7])
print(s[7:])
```

## 字符串不能直接修改：25-35 分钟

接下来讲一个字符串和列表非常不一样的地方。

列表是可以修改的，比如：

```python
arr = [1, 2, 3]
arr[0] = 100
print(arr)
```

这段代码没问题，结果会变成：

```python
[100, 2, 3]
```

但是字符串不能这样改。

比如：

```python
s = "hello"
s[0] = "H"
```

这段代码会报错。

为什么？因为字符串是不可变的。

“不可变”这个词听起来有点抽象，你可以先理解成：字符串里面的某一个位置，不能直接改。

那如果我就是想把 `hello` 变成 `Hello` 怎么办？

我们可以创建一个新的字符串：

```python
s = "hello"
s = "H" + s[1:]
print(s)
```

这里的意思是：

先拿一个新的 `"H"`，再拼上原来字符串从下标 `1` 开始到最后的部分，也就是 `"ello"`。

所以最后得到：

```python
Hello
```

我们练一个小题：

把：

```python
word = "cat"
```

变成：

```python
"bat"
```

可以这样写：

```python
word = "cat"
word = "b" + word[1:]
print(word)
```

## 常见字符串操作：35-45 分钟

现在我们来看一些常用操作。

第一个，字符串拼接。

```python
a = "hello"
b = "world"

print(a + " " + b)
```

这里的 `+` 表示把字符串连起来。中间的 `" "` 是一个空格。

输出是：

```python
hello world
```

第二个，字符串重复。

```python
print("ha" * 3)
```

输出：

```python
hahaha
```

第三个，判断一个字符串里面有没有某段内容。

```python
s = "I love Python"

print("Python" in s)
print("Java" in s)
```

第一个是 `True`，第二个是 `False`。

第四个，大小写转换。

```python
s = "hello"

print(s.upper())
print(s.lower())
```

`upper()` 是全部变大写，`lower()` 是全部变小写。

第五个，去掉首尾空格。

```python
s = "  hello  "
print(s.strip())
```

`strip()` 会去掉字符串开头和结尾的空格。

第六个，查找。

```python
s = "banana"
print(s.find("na"))
```

结果是 `2`，因为第一次出现 `"na"` 的位置是下标 `2`。

第七个，替换。

```python
s = "I like Java"
print(s.replace("Java", "Python"))
```

输出：

```python
I like Python
```

第八个，分割字符串。

```python
s = "apple,banana,orange"
fruits = s.split(",")
print(fruits)
```

输出：

```python
['apple', 'banana', 'orange']
```

注意，`split()` 会把字符串切开，变成一个列表。

## 综合练习：45-55 分钟

我们做几个小练习。

### 练习 1：统计字符个数

题目是：统计字符串里面有多少个字母 `a`。

比如：

```python
s = "banana"
```

答案应该是 `3`。

我们可以这样想：

先准备一个计数器 `count`，一开始是 `0`。然后遍历字符串，每看到一个 `a`，就让 `count` 加一。

代码：

```python
s = "banana"
count = 0

for ch in s:
    if ch == "a":
        count += 1

print(count)
```

这里的 `for ch in s` 意思是：一个字符一个字符地看。

### 练习 2：判断回文字符串

回文字符串就是正着读和倒着读一样。

比如：

```python
"level"
"madam"
```

我们刚刚学过，`s[::-1]` 可以把字符串倒过来。

所以判断回文就很简单：

```python
s = "level"

if s == s[::-1]:
    print("是回文")
else:
    print("不是回文")
```

如果原字符串和倒过来的字符串一样，它就是回文。

### 练习 3：简单用户名检查

题目是：如果用户名长度小于 3，输出“太短”；否则输出“可以使用”。

代码：

```python
username = "tom"

if len(username) < 3:
    print("太短")
else:
    print("可以使用")
```

这里我们用 `len(username)` 得到用户名长度。

## 洛谷例题讲法：可选拓展

如果这节课后要给学生上机练习，可以选洛谷的题。

### P5733 自动修正

这题要求把输入的字符串全部转成大写。

你可以对学生说：

这题其实就是考我们刚刚讲的 `upper()`。所以最直接的写法是：

```python
s = input()
print(s.upper())
```

如果学生已经比较熟，可以再问：不用 `upper()` 能不能做？

这时候就可以引入 `ord()` 和 `chr()`，不过这属于拓展，不要求一开始就掌握。

### P5015 标题统计

这题要求统计标题里面有多少个非空格字符。

可以这样引导：

我们先读入一整行，然后一个字符一个字符看。只要这个字符不是空格，就让计数器加一。

```python
s = input()
count = 0

for ch in s:
    if ch != " ":
        count += 1

print(count)
```

这题非常适合练习字符串遍历和计数。

### P1914 凯撒密码

这题比前两题难一点。它要求把每个小写字母向后移动 `n` 位。

可以这样讲：

字母表可以看成一个圈，`z` 后面又回到 `a`。所以我们需要用取模 `%`。

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

这题适合放在学生已经会遍历字符串之后。

### P1308 统计单词数

这题更适合作为挑战题。

它的关键是：

1. 不区分大小写，所以要用 `lower()`。
2. 要完整匹配单词，所以不能只用 `in`。
3. 可以先用 `split()` 把文章切成一个个单词。

简化版可以先这样写：

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

完整题还要输出第一次出现的位置，所以难度会高一些。

## 课堂收尾：55-60 分钟

最后我们总结一下。

今天我们学了字符串。字符串就是一串字符。

它和列表很像，可以：

```python
s[0]
s[1:4]
len(s)
for ch in s:
    print(ch)
```

但是字符串和列表也有一个重要区别：字符串不能直接修改某一个位置。

也就是说，这样写不行：

```python
s[0] = "A"
```

如果要修改字符串，我们通常是创建一个新的字符串。

今天还学了几个常用方法：

```python
upper()
lower()
strip()
find()
replace()
split()
```

你现在可以先记住一句话：

字符串就是一串字符。它可以访问、切片、遍历，但不能直接修改。

课后可以先做两个洛谷题：

1. P5733 自动修正
2. P5015 标题统计

这两题最适合用来巩固今天的内容。

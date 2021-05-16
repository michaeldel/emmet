#!/usr/bin/env bash
set -o nounset

EMMET=${EMMET:=./emmet}

ret=0;

tc() {
    local cmd="$EMMET"

    if [ $# -gt 2 ]; then
        while [ "$1" != '--' ]; do
            cmd="$cmd $1"
            shift
        done

        shift
    fi

    output=$(echo "$1" | $cmd)

    if [ $? -ne 0 ]; then
        echo "ERROR $1"
        ret=1
        return
    fi

    diff --color -u <(echo "$2" | sed -e '/^$/d') <(echo "$output") && echo "ok $1" || {
        echo "FAIL $1"
        ret=1
    }
}

tc 'html' '<html></html>'
tc 'nav>ul>li' '
<nav>
  <ul>
    <li></li>
  </ul>
</nav>
'
tc 'nav#menu>ul>li.item' '
<nav id="menu">
  <ul>
    <li class="item"></li>
  </ul>
</nav>
'
tc 'section>p+p+p' '
<section>
  <p></p>
  <p></p>
  <p></p>
</section>
'
tc 'section>header>h1^footer' '
<section>
  <header>
    <h1></h1>
  </header>
  <footer></footer>
</section>
'
tc 'section>(header>nav>ul>li)+footer>p' '
<section>
  <header>
    <nav>
      <ul>
        <li></li>
      </ul>
    </nav>
  </header>
  <footer>
    <p></p>
  </footer>
</section>
'
tc 'ul.menu>li.menu__item+li#id_item+li.menu__item#id_2' '
<ul class="menu">
  <li class="menu__item"></li>
  <li id="id_item"></li>
  <li class="menu__item" id="id_2"></li>
</ul>
'
tc 'input[type="text"]' '<input type="text"/>'
tc 'div[data-attr="test"]' '<div data-attr="test"></div>'

tc 'p{Lorem ipsum}' '<p>Lorem ipsum</p>'

tc '.default-block' '<div class="default-block"></div>'
tc 'em>.default-inline' '<em><span class="default-inline"></span></em>'
tc 'ul>.default-list' '
<ul>
  <li class="default-list"></li>
</ul>
'
tc 'table>.default-table-row>.default-table-column' '
<table>
  <tr class="default-table-row">
    <td class="default-table-column"></td>
  </tr>
</table>
'

tc 'ul>li*3' '
<ul>
  <li></li>
  <li></li>
  <li></li>
</ul>
'
tc 'ul>li.item$*3' '
<ul>
  <li class="item1"></li>
  <li class="item2"></li>
  <li class="item3"></li>
</ul>
'

tc 'ul>li.item$$*3' '
<ul>
  <li class="item01"></li>
  <li class="item02"></li>
  <li class="item03"></li>
</ul>
'

tc 'ul>li.item$@-*3' '
<ul>
  <li class="item3"></li>
  <li class="item2"></li>
  <li class="item1"></li>
</ul>
'

tc 'ul>li.item$@3*5' '
<ul>
  <li class="item3"></li>
  <li class="item4"></li>
  <li class="item5"></li>
  <li class="item6"></li>
  <li class="item7"></li>
</ul>
'

# exhaustive default names
tc '.foo' '<div class="foo"></div>'
tc 'b>.foo' '<b><span class="foo"></span></b>'

tc 'p>.foo' '<p><span class="foo"></span></p>'

tc 'ul>.foo' '
<ul>
  <li class="foo"></li>
</ul>
'
tc 'ol>.foo' '
<ol>
  <li class="foo"></li>
</ol>
'

tc 'table>.foo' '
<table>
  <tr class="foo"></tr>
</table>
'
tc 'tr>.foo' '
<tr>
  <td class="foo"></td>
</tr>
'
tc 'tbody>.foo' '
<tbody>
  <tr class="foo"></tr>
</tbody>
'
tc 'thead>.foo' '
<thead>
  <tr class="foo"></tr>
</thead>
'
tc 'tfoot>.foo' '
<tfoot>
  <tr class="foo"></tr>
</tfoot>
'

tc 'colgroup>.foo' '
<colgroup>
  <col class="foo"></col>
</colgroup>
'
tc 'select>.foo' '
<select>
  <option class="foo"></option>
</select>
'
tc 'optgroup>.foo' '
<optgroup>
  <option class="foo"></option>
</optgroup>
'

tc 'audio>.foo' '
<audio>
  <source class="foo"></source>
</audio>
'
tc 'video>.foo' '
<video>
  <source class="foo"></source>
</video>
'

tc 'object>.foo' '
<object>
  <param class="foo"></param>
</object>
'

tc 'map>.foo' '
<map>
  <area class="foo"></area>
</map>
'

# emmet source code tests
tc 'p' '<p></p>'
tc 'p{text}' '<p>text</p>'
tc 'h$' '<h1></h1>'
tc '.nav' '<div class="nav"></div>'
tc 'div.width1\/2' '<div class="width1/2"></div>'
tc '#sample*3' '
<div id="sample"></div>
<div id="sample"></div>
<div id="sample"></div>
'

tc 'li[repeat.for="todo of todoList"]' '<li repeat.for="todo of todoList"></li>'

tc -m sgml -n -- 'a>b' '<a><b></b></a>'
tc -m sgml -n -- 'a+b' '<a></a><b></b>'
tc -m sgml -n -- 'a+b>c+d' '<a></a><b><c></c><d></d></b>'
tc -m sgml -n -- 'a>b>c+e' '<a><b><c></c><e></e></b></a>'
tc -m sgml -n -- 'a>b>c^d' '<a><b><c></c></b><d></d></a>'
tc -m sgml -n -- 'a>b>c^^^^d' '<a><b><c></c></b></a><d></d>'
tc -m sgml -n -- 'a:b>c' '<a:b><c></c></a:b>'

tc 'ul.nav[title="foo"]' '<ul class="nav" title="foo"></ul>'

# emmet.io documentation examples
tc '#page>div.logo+ul#navigation>li*5>a{Item $}' '
<div id="page">
  <div class="logo"></div>
  <ul id="navigation">
    <li><a href="">Item 1</a></li>
    <li><a href="">Item 2</a></li>
    <li><a href="">Item 3</a></li>
    <li><a href="">Item 4</a></li>
    <li><a href="">Item 5</a></li>
  </ul>
</div>
'

tc 'div>ul>li' '
<div>
  <ul>
    <li></li>
  </ul>
</div>
'
tc 'div+p+bq' '
<div></div>
<p></p>
<blockquote></blockquote>
'
tc 'div+div>p>span+em' '
<div></div>
<div>
  <p><span></span><em></em></p>
</div>
'
tc 'div+div>p>span+em^bq' '
<div></div>
<div>
  <p><span></span><em></em></p>
  <blockquote></blockquote>
</div>
'
tc 'div+div>p>span+em^^^bq' '
<div></div>
<div>
  <p><span></span><em></em></p>
</div>
<blockquote></blockquote>
'
tc 'ul>li*5' '
<ul>
  <li></li>
  <li></li>
  <li></li>
  <li></li>
  <li></li>
</ul>
'
tc 'div>(header>ul>li*2>a)+footer>p' '
<div>
  <header>
    <ul>
      <li><a href=""></a></li>
      <li><a href=""></a></li>
    </ul>
  </header>
  <footer>
    <p></p>
  </footer>
</div>
'
tc '(div>dl>(dt+dd)*3)+footer>p' '
<div>
  <dl>
    <dt></dt>
    <dd></dd>
    <dt></dt>
    <dd></dd>
    <dt></dt>
    <dd></dd>
  </dl>
</div>
<footer>
  <p></p>
</footer>
'
tc 'div#header+div.page+div#footer.class1.class2.class3' '
<div id="header"></div>
<div class="page"></div>
<div id="footer" class="class1 class2 class3"></div>
'
tc 'td[title="Hello world!" colspan=3]' '
<td title="Hello world!" colspan="3"></td>
'
tc 'ul>li.item$*5' '
<ul>
  <li class="item1"></li>
  <li class="item2"></li>
  <li class="item3"></li>
  <li class="item4"></li>
  <li class="item5"></li>
</ul>
'
tc 'ul>li.item$$$*5' '
<ul>
  <li class="item001"></li>
  <li class="item002"></li>
  <li class="item003"></li>
  <li class="item004"></li>
  <li class="item005"></li>
</ul>
'
tc 'ul>li.item$@-*5' '
<ul>
  <li class="item5"></li>
  <li class="item4"></li>
  <li class="item3"></li>
  <li class="item2"></li>
  <li class="item1"></li>
</ul>
'
tc 'ul>li.item$@3*5' '
<ul>
  <li class="item3"></li>
  <li class="item4"></li>
  <li class="item5"></li>
  <li class="item6"></li>
  <li class="item7"></li>
</ul>
'
tc 'ul>li.item$@-3*5' '
<ul>
  <li class="item7"></li>
  <li class="item6"></li>
  <li class="item5"></li>
  <li class="item4"></li>
  <li class="item3"></li>
</ul>
'
tc 'a{Click me}' '<a href="">Click me</a>'
tc 'a>{Click me}' '<a href="">Click me</a>'
tc 'a{click}+b{here}' '<a href="">click</a><b>here</b>'
tc 'a>{click}+b{here}' '<a href="">click<b>here</b></a>'
tc 'p>{Click }+a{here}+{ to continue}' '
<p>Click <a href="">here</a> to continue</p>
'
tc 'p{Click }+a{here}+{ to continue}' '
<p>Click </p>
<a href="">here</a> to continue
'

# details
tc 'a#foo' '<a href="" id="foo"></a>'

# simple SGML examples
tc -m sgml -- 'a' '<a></a>'
tc -m sgml -- 'a>b' '
<a>
  <b></b>
</a>
'
tc -m sgml -- 'a>b>c' '
<a>
  <b>
    <c></c>
  </b>
</a>
'
tc -m sgml -- 'a>(b>c)+d' '
<a>
  <b>
    <c></c>
  </b>
  <d></d>
</a>
'
tc -m sgml -- '(a+b)*2' '
<a></a>
<b></b>
<a></a>
<b></b>
'
tc -m sgml -- '(a+b)*2+c' '
<a></a>
<b></b>
<a></a>
<b></b>
<c></c>
'

tc -m sgml -- 'a*0' '<a></a>'
tc -m sgml -- 'a.item$*10' '
<a class="item1"></a>
<a class="item2"></a>
<a class="item3"></a>
<a class="item4"></a>
<a class="item5"></a>
<a class="item6"></a>
<a class="item7"></a>
<a class="item8"></a>
<a class="item9"></a>
<a class="item10"></a>
'
tc -m sgml -- 'a.item$@10*3' '
<a class="item10"></a>
<a class="item11"></a>
<a class="item12"></a>
'
tc -m sgml -- 'a{foo $}*3' '
<a>foo 1</a>
<a>foo 2</a>
<a>foo 3</a>
'
tc -m sgml -- 'a.\$*2' '
<a class="$"></a>
<a class="$"></a>
'

tc -m sgml -- 'a*2>b{item$}' '
<a>
  <b>item1</b>
</a>
<a>
  <b>item2</b>
</a>
'
tc -m sgml -- 'a*2+b' '
<a></a>
<a></a>
<b></b>
'
tc -m sgml -- 'a*2>b+c' '
<a>
  <b></b>
  <c></c>
</a>
<a>
  <b></b>
  <c></c>
</a>
'

tc -m sgml -- '(a.$*2+b)*2' '
<a class="1"></a>
<a class="2"></a>
<b></b>
<a class="1"></a>
<a class="2"></a>
<b></b>
'
tc -m sgml -- '(a.\$*2+b)*2' '
<a class="$"></a>
<a class="$"></a>
<b></b>
<a class="$"></a>
<a class="$"></a>
<b></b>
'

tc -m sgml -- '(a.$*2)*2' '
<a class="1"></a>
<a class="2"></a>
<a class="1"></a>
<a class="2"></a>
'

tc -m sgml -- 'a.foo#bar' '<a class="foo" id="bar"></a>'
tc -m sgml -- 'a#foo.bar' '<a id="foo" class="bar"></a>'
tc -m sgml -- 'a.foo.bar' '<a class="foo bar"></a>'
tc -m sgml -- 'a.foo.bar.baz' '<a class="foo bar baz"></a>'

tc -m sgml -- 'a[foo="bar"]' '<a foo="bar"></a>'
tc -m sgml -- 'a[foo=bar]' '<a foo="bar"></a>'
tc -m sgml -- 'a[foo="bar baz"]' '<a foo="bar baz"></a>'
tc -m sgml -- 'a[one=1 two=2]' '<a one="1" two="2"></a>'

tc -m sgml -- 'a^^b' '
<a></a>
<b></b>
'
tc -m sgml -- 'a^^^^^^^^b' '
<a></a>
<b></b>
'
tc -m sgml -- 'a^^b^^c' '
<a></a>
<b></b>
<c></c>
'

exit $ret

#!/usr/bin/env bash
EMMET=${EMMET:=./emmet}

ret=0;

tc() {
    output=$(echo "$1" | $EMMET)

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

tc 'a' '<a></a>'
tc 'a>b' '
<a>
  <b></b>
</a>
'
tc 'a>b>c' '
<a>
  <b>
    <c></c>
  </b>
</a>
'
tc 'a>(b>c)+d' '
<a>
  <b>
    <c></c>
  </b>
  <d></d>
</a>
'
tc 'a.item$*10' '
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
tc 'a.item$@10*3' '
<a class="item10"></a>
<a class="item11"></a>
<a class="item12"></a>
'
tc 'a{foo $}*3' '
<a>foo 1</a>
<a>foo 2</a>
<a>foo 3</a>
'

tc 'a.foo#bar' '<a class="foo" id="bar"></a>'
tc 'a#foo.bar' '<a id="foo" class="bar"></a>'
tc 'a.foo.bar' '<a class="foo bar"></a>'
tc 'a.foo.bar.baz' '<a class="foo bar baz"></a>'

exit $ret

#!/usr/bin/env bash
ret=0;

tc() {
    output=$(echo "$1" | ./emmet)
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
exit $ret

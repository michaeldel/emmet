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
exit $ret

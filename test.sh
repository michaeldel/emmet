#!/usr/bin/env bash
ret=0;

tc() {
    output=$(echo "$1" | ./emmet)
    diff -u <(echo "$2" | sed -e '/^$/d') <(echo "$output") && echo "ok $1" || {
        "FAIL $2"
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

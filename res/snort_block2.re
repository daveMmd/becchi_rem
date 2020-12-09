#(back end) state explosion
#
(\r\nContent-Disposition\x3a\s+form-data\x3b[^\r\n]+\r\n\r\n.+){250}
^[^\d]{1,20}100[^\d]{1,20}101[^\d]{1,20}102[^\d]{1,20}97[^\d]{1,20}117[^\d]{1,20}108[^\d]{1,20}116[^\d]{1,20}35[^\d]{1,20}86[^\d]{1,20}77[^\d]{1,20}76
#
#08.30
#
#anchor rule become more expensive to extract middle part
#^[\x20-\x7e]+.{8}\x7d\x94
#"()"子表达式语义处理缺陷，后续可改进
#p_match of one anchor rule is caculated wrongly
^.{4}[\x0a-\x7f]{0,100}[\x00-x09\x80-\xff]
#
#degrade performance greatly
#
[\w\W]{2500,}
[0-9a-zA-Z]{200,}
[a-zA-Z0-9]{1000,}
[\w]{70,}
#
#generate too many prefix matches
#
^.*[\x24\x60]
\s+.*%.*%
=.*[a-f].*$
.*\[.*\].*\;
^[^\x22]*[\x24\x60]+
^[A-F0-9]+\x00
^[^\x00]+\x00
\x7C\x7C.+[a-z]
^[^\x27]+[\x27]\s*
^..[^\x00]+\x00
^.{4}[^\x00]+\x00
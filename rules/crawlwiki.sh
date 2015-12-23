#!/bin/bash

#set -x

PAGELIST=pagelist2

DO_PAGELIST=1
DO_RAW=1
DO_JSON=1
DO_HTML0=1
DO_DOCBOOK=0
DO_CONCAT=0
DO_HTML1=1
DO_PDF=1

main_title="Eressea-Regeln"

PAPER_SIZE=a4

verbose=3

ERROR_LEVEL=1
WARNING_LEVEL=2
INFO_LEVEL=3
DEBUG_LEVEL=4

CREATE_DIRECTORIES=1

URL=http://wiki.eressea.de/index.php?title=
RAWACTION="&redirect=no&action=raw"
PAGELIST_URL="http://wiki.eressea.de/index.php?title=Spezial%3AAlle+Seiten&from=&to=&namespace=0"
REDIRECT_OPTION="&hideredirects=1"

RAW_DIR=mediawiki
JSON_DIR=json
DOCBOOK_DIR=docbook
HTML_DIR=html
LATEX_DIR=latex

DOC_EXT=.xml
JSON_EXT=.json
WIKI_EXT=.mediawiki
HTML_EXT=.html

charset=utf-8

raw_pagelist=pagelist.raw$HTML_EXT
all_pagelist=pagelist.all
real_pagelist=pagelist.real
redirect_pagelist=pagelist.redirect

html0_contentsfile=$HTML_DIR/Inhalt$HTML_EXT
html0_templatefile=wikitemplate0.html
html1_templatefile=wikitemplate0.html
html1_tmpfile=$HTML_DIR/html1_tmp$HTML_EXT
html1_file=$HTML_DIR/Regeln_komplett$HTML_EXT

latex_file=$LATEX_DIR/Regeln_komplett.tex

concat_file="$DOCBOOK_DIR/allpages.xml"

function check_file {
    if [ ! -r "$1" ]; then
	if ((verbose>=ERROR_LEVEL)); then
	    echo "File $1 is not readable, aborting."
	fi
	exit -1
    fi
}

function check_dir {
    if [ ! -d "$1" ]; then
	if ((CREATE_DIRECTORIES!=0)); then
	    mkdir "$1"
	fi
    fi
    if [ ! -d "$1" ]; then
	if ((verbose>=ERROR_LEVEL)); then
	    echo "Directory $1 does not exist, aborting."
	fi
	exit -1
    fi
}

function link_id {
    id=$(echo -n "$1" | tr -c -s '[a-z][A-Z][0-9][_.-]' '_')
    python -c "from ewiki import link_id; print link_id('', '$id')"
}

function url_encode {
    python -c "from ewiki import url_encode; print url_encode('$1')"
}

function html_encode {
    python -c "from ewiki import html_encode; print html_encode(u'$1')"
}

function page_titles {
    python -c "from ewiki import page_list; print('#cwsh#'.join(page_list('$1')))"
}

redirects=""

function add_redirect {
    if [ -z "$redirects" ]; then
	redirects="{ \"$1\": \"$2\" }"
    else
	redirects="${redirects:0:-1}, \"$1\": \"$2\" }"
    fi
}

function echo_usage {
    doit=$1
    if ((doit==1)); then
	if ((verbose>=INFO_LEVEL)); then
 	    echo $2
	fi
	check_dir $3
    fi
}

echo_usage DO_RAW "downloading mediawiki into $RAW_DIR" $RAW_DIR
echo_usage DO_JSON "creating raw json in $JSON_DIR" $JSON_DIR
echo_usage DO_DOCBOOK "creating docbook in $DOCBOOK_DIR" $DOCBOOK_DIR
echo_usage DO_HTML0 "creating html docs in $HTML_DIR" $HTML_DIR
echo_usage DO_HTML1 "creating big html in $HTML_DIR" $HTML_DIR
echo_usage DO_PDF "creating pdf in $LATEX_DIR" $LATEX_DIR

if ((DO_PAGELIST==1)); then
    curl -s -f -m60 "$PAGELIST_URL" > "$raw_pagelist"
    echo $(page_titles "$raw_pagelist") | sed -e"s/#cwsh#/\n/g" > "$all_pagelist"
    curl -s -f -m60 "$PAGELIST_URL$REDIRECT_OPTION" > "$raw_pagelist"
    echo $(page_titles "$raw_pagelist") | sed -e"s/#cwsh#/\n/g" > "$real_pagelist"
    diff pagelist.real pagelist.all  | grep ">" | sed -e "s/> //" > "$redirect_pagelist"
    cat "$all_pagelist" | sort -u > .pl.cwsh1
    cat pagelist2 | grep -v "^###" | grep -v "^\$"| cut -f2 | sort -u > .pl.cwsh2
    if ((verbose>=WARNING_LEVEL)); then
	echo "missing pages:"
	diff .pl.cwsh1 .pl.cwsh2
	echo "---"
    fi
    rm  .pl.cwsh1 .pl.cwsh2
    cat "$redirect_pagelist" | sort -u > .pl.cwsh1
    cat pagelist2 | grep "^R" | cut -f2 | sort -u > .pl.cwsh2
    if ((verbose>=WARNING_LEVEL)); then
	echo "missing redirects:"
	diff .pl.cwsh1 .pl.cwsh2
	echo "---"
    fi
    rm  .pl.cwsh1 .pl.cwsh2
fi

if ((DO_CONCAT==1)); then
    if [ -e "$concat_file" ]; then
	if ((verbose>=ERROR_LEVEL)); then
	    echo "$concat_file exists, aborting."
	fi
	exit -1
    fi
    touch "$concat_file"
fi

if ((DO_HTML0==1)); then
    echo "<html><head>"  > "$html0_contentsfile"
    echo "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=$charset\">"  >> "$html0_contentsfile"
    echo "<title>$main_title: Inhalt</title>" >> "$html0_contentsfile"
    echo "</head><body>" >> "$html0_contentsfile"
    echo "<b>Inhalt</b><br /><div id='content'><ul>" >> "$html0_contentsfile"
fi

if ((DO_HTML1==1)); then
    rm -f "$html1_tmpfile"
fi

cat $PAGELIST | while read pageline; do
    if [ -z "$pageline" ]; then
	continue
    fi
    if [ ${pageline:0:1} = "#" ]; then
	if [ ${pageline:0:2} != "###" ]; then
	    if ((verbose>=INFO_LEVEL)); then
		echo "skipping $pageline"
	    fi
	fi
	continue;
    fi
    IFS='	' read -r -a linearray <<< "$pageline"
    depth=${linearray[0]}

    page=${linearray[1]}
    pageurl=$(url_encode "$page")

    title=${linearray[2]}
    if [ -z "$title" ]; then
	title=$page
    fi

    page_id=${linearray[3]}
    if [ -z "$page_id" ]; then
	page_id=$page
    fi
    page_id=$(link_id "$page_id")

    
    if ((verbose>=INFO_LEVEL)); then
	if [ "$depth" = "R" ]; then
	    echo "adding redirect $pageline"
	else
	    echo "processing $pageurl at depth $depth, named '$title' ($page_id)"
	fi
    fi

    rawfile=$RAW_DIR/$page$WIKI_EXT
    jsonfile=$JSON_DIR/$page$JSON_EXT
    htmlfile=$HTML_DIR/$(url_encode "$page$HTML_EXT")
    outputfile=$DOCBOOK_DIR/$page$DOC_EXT
    
    if ((DO_RAW==1)); then
	#wget -o "$RAW_DIR/$page$LOG_EXT" --output-document="$rawfile" "$URL$pageurl$RAWACTION"
        curl -s -f -m60 -o "$rawfile" "$URL$pageurl$RAWACTION"
        status=$?
        if (($status!=0)); then
	    if ((verbose>=WARNING_LEVEL)); then
		echo "error downloading $page, exit code $status."
	    fi
        fi
	# insert h1 header
	# sed -i 1i"= $title =\n" "$rawfile"
    fi

    if [ "$depth" = "R" ]; then
	redirect=$(pandoc -f mediawiki -t plain --filter=ewiki_filter_redirect.py "$rawfile")
	add_redirect "$page" "$redirect" 
	continue
    fi

    if ((DO_JSON==1)); then
	check_file "$rawfile"
	pandoc -f mediawiki -t json -o "$jsonfile" "$rawfile" 
    fi

    if ((DO_HTML0==1)); then
	if ((verbose>=DEBUG_LEVEL)); then
	    echo "---$redirects--"
	fi
	cat "$jsonfile" | 
	pandoc -f json -t html -M id-prefix="$page_id." -M redirects="$redirects" --filter=ewiki_filter_html0.py \
	    --id-prefix="$page_id." -s -V title="$title" -V pagetitle="$title" -V css="common.css" \
	    -V contents="Inhalt.html" --template="$html0_templatefile" --toc --toc-depth=2 \
	    > "$htmlfile" 
	url=$(url_encode "$page")
	url=$(url_encode "$url$HTML_EXT")
        echo "<li><a href="$url">$title</a></li>" >> "$html0_contentsfile"
    fi

    if ((DO_HTML1==1)); then
	if ((verbose>=DEBUG_LEVEL)); then
	    echo "---$redirects--"
	fi
	echo "<h1 id=\"$page_id.\">$title</h1>" >> "$html1_tmpfile"
	cat "$jsonfile" | 
	pandoc -f json -t html -M id-prefix="$page_id." -M redirects="$redirects" --filter=ewiki_filter_html1.py \
	    --id-prefix="$page_id." \
	    >> "$html1_tmpfile"
	echo "" >> "$html1_tmpfile"
    fi

    if ((DO_DOCBOOK==1)); then
	cat "$jsonfile" | 
	  python ewiki_filter.py | 
	  pandoc -f json -t docbook -s --id-prefix="$page_id." --template=eresseawikidoc.template \
	      --base-header-level=$((depth+1)) -V title="$title" |
	  # replace sect1 by section
	  sed -e "s/sect[0-9]/section/" \
	      > "$outputfile" 
    fi

    if ((DO_CONCAT==1)); then
	check_file "$outputfile"
	cat "$outputfile" >> "$concat_file"
    fi
done


if ((DO_HTML0==1)); then
    echo "</ul></body></html>" >> "$html0_contentsfile"
    cp common.css "$HTML_DIR"
fi

if ((DO_HTML1==1)); then
    cat "$html1_tmpfile" |
    pandoc -f html -t html -M id-prefix="$page_id." -s \
	-V title="$main_title" -V pagetitle="$main_title" -V css="common.css" \
	--template="$html1_templatefile" --toc --toc-depth=2 \
	> "$html1_file"
    cp common.css "$HTML_DIR"
    # rm -f "$html1_tmpfile"
fi

if ((DO_PDF==1)); then
    if ((verbose>=INFO_LEVEL)); then
	echo "creating tex file"
    fi
    check_file "$html1_tmpfile"
    cat "$html1_tmpfile" |
    pandoc -f html -t latex -s -M title="$main_title" \
	-V title="$main_title" -V linkcolor=blue -V lot=true -V papersize=$PAPER_SIZE \
	--toc --toc-depth=2 --id-prefix="ewiki." \
	> "$latex_file"
    status=$?
    if ((verbose>=INFO_LEVEL)); then
	echo "creating pdf file"
    fi
    if (($status==0)); then    
	pdflatex --output-directory "$LATEX_DIR" -halt-on-error "$latex_file" > /dev/null && \
	    pdflatex --output-directory "$LATEX_DIR" -halt-on-error "$latex_file" > /dev/null && \
	    pdflatex --output-directory "$LATEX_DIR" -halt-on-error "$latex_file" > /dev/null
    fi
fi


# works:
# mediawiki2latex -o eresseawiki.pdf -c eresseatree -u http://localhost/testwiki/index.php/Regeln_\(komplett\)

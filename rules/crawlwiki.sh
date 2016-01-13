#!/bin/bash

#set -x


DO_PAGELIST=1
DO_RAW=0
DO_JSON=1
DO_HTML0=1
DO_DOCBOOK=0
DO_CONCAT=0
DO_HTML1=1
DO_PDF=1
DO_CLEAN=0

PAGELIST=pagelist2
#PAGELIST=pagelist_en
LANGUAGE_SUFFIX=
#LANGUAGE_SUFFIX=_en

main_title="Eressea-Regeln"
#main_title="Eressea Rules"

PAPER_SIZE=a4paper
LATEX_LANGUAGE=de
#LATEX_LANGUAGE=en
LATEX_BABEL_LANG=german
#LATEX_BABEL_LANG=english

MEDIAWIKI_TAGS='{ "keep": ["blockquote","br","center","div","noinclude","nowiki","pre","s","span","sub","sup"] , "remove": ["tt", "code"] }'

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

RAW_DIR=mediawiki$LANGUAGE_SUFFIX
JSON_DIR=json$LANGUAGE_SUFFIX
DOCBOOK_DIR=docbook$LANGUAGE_SUFFIX
HTML_DIR=html$LANGUAGE_SUFFIX
LATEX_DIR=latex$LANGUAGE_SUFFIX
TEMPLATE_DIR=templates
FILTERS_DIR=filters

DOC_EXT=.xml
JSON_EXT=.json
WIKI_EXT=.mediawiki
HTML_EXT=.html
TMP_EXT=.tmp

charset=utf-8

raw_pagelist=pagelist.raw$HTML_EXT
all_pagelist=pagelist.all
real_pagelist=pagelist.real
redirect_pagelist=pagelist.redirect

redirect_file=$RAW_DIR/redirects.tmp
concat_file=$JSON_DIR/concat.json

html0_contentsfile=$HTML_DIR/Inhalt$HTML_EXT
html0_templatefile=$TEMPLATE_DIR/wikitemplate0.html
html1_templatefile=$TEMPLATE_DIR/wikitemplate0.html
html1_file=$HTML_DIR/Regeln_komplett$HTML_EXT

doc_templatefile=$TEMPLATE_DIR/wikitemplate.docbook

latex_file=$LATEX_DIR/Regeln_komplett.tex
latex_templatefile=$TEMPLATE_DIR/wikitemplate.tex

export PYTHONPATH=$PYTHONPATH:lib

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
#    id=$(echo -n "$1" | tr -c -s '[a-z][A-Z][0-9][_.-]' '_')
    id="$1"
    python -c "from ewiki import link_id; print link_id(None, u'$id')"
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

function add_redirect {
    if [ -z "$redirects" ]; then
	redirects="{ \"$1\": \"$2\" }"
    elif [ "$redirects" = "{}" ]; then
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

function remove_tags {
    filename=$1
    tags=$2
    python -c "from ewiki import remove_tags; remove_tags('$filename', '$tags')"
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

if ((DO_HTML0==1)); then
    echo "<html><head>"  > "$html0_contentsfile"
    echo "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=$charset\">"  >> "$html0_contentsfile"
    echo "<title>$main_title: Inhalt</title>" >> "$html0_contentsfile"
    echo "</head><body>" >> "$html0_contentsfile"
    echo "<b>Inhalt</b><br /><div id='content'><ul>" >> "$html0_contentsfile"
fi

DO_CONCAT=0

if ((DO_HTML1==1)); then
    DO_CONCAT=1
fi

if ((DO_PDF==1)); then
    DO_CONCAT=1
fi

if ((DO_CONCAT==1)); then
    rm -f "$concat_file"
fi

redirects="{}"

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

    redirect=${linearray[2]}
    title=${linearray[2]}
    if [ -z "$title" ]; then
	title=$page
    fi

    page_id=${linearray[3]}
    if [ -z "$page_id" ]; then
	page_id=$page
    fi
#    page_id=$(link_id "$page_id")
#    echo $page_id

    
    if ((verbose>=INFO_LEVEL)); then
	if [ "$depth" = "R" ]; then
	    echo "adding redirect $pageline"
	else
	    echo "processing $pageurl at depth $depth, named '$title' ($page_id)"
	fi
    fi

    rawfile=$RAW_DIR/$page$WIKI_EXT
    jsonfile=$JSON_DIR/$page$JSON_EXT
    json_tmpfile=$JSON_DIR/$page$TMP_EXT$JSON_EXT
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
	if [ -z "$redirect" ]; then
	    redirect=$(pandoc -f mediawiki -t plain --filter="$FILTERS_DIR/ewiki_filter_redirect.py" "$rawfile")
	fi
	add_redirect "$page" "$redirect" 
	continue
    fi

    if ((DO_JSON==1)); then
	check_file "$rawfile"
	remove_tags "$rawfile" "$MEDIAWIKI_TAGS" |
	pandoc -f mediawiki -t json -o "$jsonfile" 
    fi

    if ((DO_HTML0==1)); then
	if ((verbose>=DEBUG_LEVEL)); then
	    echo "---$redirects--"
	fi
	#"$jsonfile" | 
	cat "$jsonfile" |
	pandoc -f json -t html -M id-prefix="$page_id" -M redirects="$redirects" \
	    --filter="$FILTERS_DIR/ewiki_filter_html0.py" \
	     -s -V title="$title" -V pagetitle="$title" -V css="common.css" \
	    -V contents="Inhalt.html" --template="$html0_templatefile" --toc --toc-depth=2 \
	    > "$htmlfile" 
	url=$(url_encode "$page")
	url=$(url_encode "$url$HTML_EXT")
        echo "<li><a href="$url">$title</a></li>" >> "$html0_contentsfile"
    fi

    if ((DO_CONCAT==1)); then
	if [ ! -w "$concat_file" ]; then
	    echo "[" > "$concat_file"
	else
	    echo -n "," >> "$concat_file"
	fi
	echo "[\"$json_tmpfile\", \"$title\"]" >> "$concat_file"

	cat "$jsonfile" |
	pandoc -f json -t json -s -M id-prefix="$title" -M redirects="$redirects" \
	    --filter="$FILTERS_DIR/ewiki_filter_tex.py" \
	    > "$json_tmpfile"
    fi

    if ((DO_DOCBOOK==1)); then
	cat "$jsonfile" | 
	  python "$FILTERS_DIR/ewiki_filter_doc.py" | 
	  pandoc -f json -t docbook -s --id-prefix="$page_id." --template="$doc_templatefile" \
	      --base-header-level=$((depth+1)) -V title="$title" |
	  # replace sect1 by section
	  sed -e "s/sect[0-9]/section/" \
	      > "$outputfile" 
    fi

    echo $redirects > "$redirect_file"
done

check_file "$redirect_file"
redirects=$(cat $redirect_file)

if ((DO_HTML0==1)); then
    echo "</ul></body></html>" >> "$html0_contentsfile"
    cp $TEMPLATE_DIR/common.css "$HTML_DIR"
fi

if ((DO_CONCAT==1)); then
    echo "]" >> "$concat_file"
fi

if ((DO_HTML1==1)); then
    if ((verbose>=DEBUG_LEVEL)); then
	echo "---$redirects--"
    fi
    if ((verbose>=INFO_LEVEL)); then
	echo "creating single html file"
    fi

    check_file "$concat_file"

    python lib/concat_json.py "$concat_file" |
    pandoc -f json -t html  \
	-s -M title="$main_title" --template="$latex_templatefile" \
	-V title="$main_title" -V pagetitle="$main_title" -V css="common.css" \
	--template="$html1_templatefile" --toc --toc-depth=2 \
	> "$html1_file"
    status=$?

    cp $TEMPLATE_DIR/common.css "$HTML_DIR"
fi

if ((DO_PDF==1)); then
    if ((verbose>=DEBUG_LEVEL)); then
	echo "---$redirects--"
    fi
    if ((verbose>=INFO_LEVEL)); then
	echo "creating tex file"
    fi

    check_file "$concat_file"

    python lib/concat_json.py "$concat_file" |
    pandoc -f json -t latex  \
	-s -M title="$main_title" --template="$latex_templatefile" \
	-V title="$main_title" -V linkcolor=blue -V lot=true --toc --toc-depth=2 \
	-V lang=$LATEX_LANGUAGE -V babel-lang=$LATEX_BABEL_LANG -V papersize=$PAPER_SIZE \
	-V geometry="top=2cm, bottom=2cm, left=1.5cm, right=1.5cm" \
	--id-prefix="ewiki." \
	> "$latex_file"
    status=$?
    if ((verbose>=INFO_LEVEL)); then
	echo "creating pdf file"
    fi
    if (($status==0)); then    
	pdflatex --output-directory "$LATEX_DIR" -interaction nonstopmode "$latex_file" > /dev/null && \
	    pdflatex --output-directory "$LATEX_DIR" -interaction nonstopmode "$latex_file" > /dev/null && \
	    pdflatex --output-directory "$LATEX_DIR" -interaction nonstopmode "$latex_file" > /dev/null
	status=$?
	if (($status!=0)); then
	    if ((verbose>=ERROR_LEVEL)); then
		echo "error creating pdf file, see $LATEX_DIR/*.log"
		echo "or just try running"
		echo "pdflatex -interaction nonstopmode --output-directory $LATEX_DIR $latex_file"
	    fi
	fi
    fi
fi

if ((DO_CLEAN==1)); then
    rm $RAW_DIR/*
    rm $JSON_DIR/*
    rm $DOCBOOK_DIR/*
    rm $LATEX_DIR/*~ $LATEX_DIR/*aux $LATEX_DIR/*lot $LATEX_DIR/*out  $LATEX_DIR/*toc
    rm "$raw_pagelist"
fi

# works:
# mediawiki2latex -o eresseawiki.pdf -c eresseatree -u http://localhost/testwiki/index.php/Regeln_\(komplett\)

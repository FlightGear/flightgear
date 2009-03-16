" Vim syntax file
" Language:	Nasal (FlightGear)
" Maintainer:	Melchior FRANZ <mfranz # aon : at>
" URL:		http://members.aon.at/mfranz/nasal.vim
" Last Change:	2008 Sep 29

" ________________________________CUSTOMIZATION______________________________
"
" :let nasal_no_fgfs=1               " turn off FlightGear extensions
" :hi nasalStatement ctermfg=Green   " change statement color
" ___________________________________________________________________________
" for use in ~/.vimrc drop the initial colon
" type ":help new-filetype" in vim for installation instructions


if !exists("main_syntax")
	if version < 600
		syntax clear
	elseif exists("b:current_syntax")
		finish
	endif
	let main_syntax = 'nasal'
endif


syn keyword nasalCommentTodo		TODO FIXME XXX contained
syn match   nasalComment		"#.*$" contains=nasalCommentTodo
syn region  nasalStringS		start=+'+ skip=+\\'+ end=+'+ contains=nasalSpecialS
syn region  nasalStringD		start=+"+ skip=+\\"+ end=+"+ contains=nasalSpecialD,nasalSpecial
syn match   nasalSpecialS		contained "\\'"
syn match   nasalSpecialD		contained "\\[\\rnt\"]"
syn match   nasalSpecial		contained "\\x[[:xdigit:]][[:xdigit:]]"

syn match   nasalError			"``\="
syn match   nasalError			"`\\[^`\\rnt]`"
syn match   nasalError			"`[^`][^`]\+`"
syn match   nasalCharConstant		"`[^`\\]`"
syn match   nasalCharConstant		"`\\[`\\rnt]`"
syn match   nasalCharConstant		"`\\x[[:xdigit:]][[:xdigit:]]`"

syn match   nasalNumber			"-\=\<0x\x\+\>"
syn match   nasalNumber			"-\=\<\d\+\>"
syn match   nasalNumber			"-\=\.\d\+\([eE][+-]\=\d\+\)\=\>"
syn match   nasalNumber			"-\=\<\d\+\.\=\([eE][+-]\=\d\+\)\=\>"
syn match   nasalNumber			"-\=\<\d\+\.\d\+\([eE][+-]\=\d\+\)\=\>"

syn keyword nasalStatement		func return var
syn keyword nasalConditional		if elsif else
syn keyword nasalRepeat			while for foreach forindex
syn keyword nasalBranch			break continue
syn keyword nasalVar			me arg parents
syn keyword nasalType			nil
syn keyword nasalOperator		and or
syn match   nasalFoo			"\~"

syn match   nasalFunction		display "\<contains\>"
syn keyword nasalFunction		size keys append pop setsize subvec delete int num streq substr
syn keyword nasalFunction		chr typeof compile call die sprintf caller closure find cmp
syn keyword nasalFunction		split rand bind sort ghosttype id

" math lib
syn match   nasalFunction		"\<math\.\(sin\|cos\|exp\|ln\|sqrt\|atan2\)\>"
syn match   nasalConstant		"\<math\.\(e\|pi\)\>"

" io lib
syn match   nasalFunction		"\<io\.\(close\|read\|write\|seek\|tell\|flush\|open\|readln\|stat\)\>"
syn match   nasalVar			"\<io\.\(SEEK_SET\|SEEK_CUR\|SEEK_END\|stdin\|stdout\|stderr\)\>"

" bits lib
syn match   nasalFunction		"\<bits\.\(sfld\|fld\|setfld\|buf\)\>"


syn sync fromstart
syn sync maxlines=100

syn match   nasalParenError	"[()]"
syn match   nasalBraceError	"[{}]"
syn match   nasalBrackError	"[\[\]]"


" FlightGear specific commands
if !exists("nasal_no_fgfs")
	syn keyword nasalFGFSFunction		getprop setprop print _fgcommand settimer _setlistener _cmdarg
	syn keyword nasalFGFSFunction		_interpolate rand srand directory removelistener systime
	syn keyword nasalFGFSFunction		geodtocart carttogeod geodinfo parsexml airportinfo abort

	syn keyword nasalGlobalsFunction	isa fgcommand cmdarg abs interpolate setlistener defined printlog
	syn keyword nasalGlobalsFunction	thisfunc printf values

	syn keyword nasalPropsFunction		getType getName getIndex getValue setValue setIntValue
	syn keyword nasalPropsFunction		setBoolValue setDoubleValue getParent getChild getChildren
	syn keyword nasalPropsFunction		getAttribute setAttribute alias unalias getAliasTarget clearValue
	syn keyword nasalPropsFunction		removeChild removeChildren getNode initNode
	syn keyword nasalPropsFunction		getPath getBoolValue setValues getValues
	syn match   nasalPropsFunction		"\<props\.\(_\?globals\|Node\|nodeList\|condition\)\>\.\="

	" XML embedded mode
	if expand("%:e") == "xml"
		syn region  nasalComment	start="<!--" end="-->" contains=nasalCommentTodo
		syn region  nasalComment	start="<?" end="?>" contains=nasalCommentTodo
		syn match   nasalComment	"^\s*</\?[[:alnum:]!].*[[:alnum:]\"-]/\?>\s*$"
		syn match   nasalComment	"^\s*<script>"
		syn match   nasalComment	"</script>.*"
		syn match   nasalCDATA		"<!\[CDATA\["
		syn match   nasalCDATA		"\]\]>"
	endif
endif


syn region  nasalEncl transparent matchgroup=nasalParenEncl start="(" end=")" contains=ALLBUT,nasalParenError
syn region  nasalEncl transparent matchgroup=nasalBrackEncl start="\[" end="\]" contains=ALLBUT,nasalBrackError
syn region  nasalEncl transparent matchgroup=nasalBraceEncl start="{" end="}" contains=ALLBUT,nasalBraceError


if version >= 508 || !exists("did_nasal_syn_inits")
	if version < 508
		let did_nasal_syn_inits = 1
		command -nargs=+ HiLink hi link <args>
	else
		command -nargs=+ HiLink hi def link <args>
	endif
	HiLink nasalComment		Comment
	HiLink nasalCommentTodo		Todo
	HiLink nasalSpecial		Special
	HiLink nasalSpecialS		Special
	HiLink nasalSpecialD		Special
	HiLink nasalStringS		String
	HiLink nasalStringD		String
	HiLink nasalNumber		Number
	HiLink nasalConditional		Conditional

	HiLink nasalVar			Macro
	HiLink nasalType		Type
	HiLink nasalConstant		Constant
	HiLink nasalCharConstant	Type
	HiLink nasalFoo			NonText
	HiLink nasalCDATA		Type

	HiLink nasalRepeat		Repeat
	HiLink nasalBranch		Conditional
	HiLink nasalOperator		Operator
	HiLink nasalStatement		Statement
	HiLink nasalFunction		Function

	HiLink nasalFGFSFunction	Function
	HiLink nasalGlobalsFunction	Function
	HiLink nasalPropsFunction	Function

	HiLink nasalError		Error
	HiLink nasalParenError		nasalError
	HiLink nasalBrackError		nasalError
	HiLink nasalBraceError		nasalError
	delcommand HiLink
endif

let b:current_syntax = "nasal"
if main_syntax == 'nasal'
	unlet main_syntax
endif

" vim: ts=8

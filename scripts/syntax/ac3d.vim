if !exists("main_syntax")
  if version < 600
    syntax clear
"  elseif exists("b:current_syntax")
"    finish
  endif
  let main_syntax = 'ac3d'
endif

"setlocal iskeyword=46,95,97-122

syn keyword ac3dIdentifier      AC3Db
syn region  ac3dMaterial        start=+^MATERIAL+ end=+$+ contains=ac3dSTringS,ac3dStringD,ac3dMatKeyword
syn match   ac3dError           display +^OBJECT+
syn match   ac3dObject          display +^OBJECT\s\+\(world\|group\|poly\)\s*$+
syn match   ac3dMaterial        display +^SURF.*+
syn region  ac3dStringS         start=+'+  end=+'+
syn region  ac3dStringD         start=+"+  end=+"+
syn match   ac3dFunction        display +^\(crease\|mat\|texture\|texrep\|texoff\|url\|data\|refs\)+
syn match   ac3dFunction        display +^\(numvert\|numsurf\|kids\|name\|SURF\|loc\)+
syn keyword ac3dMatKeyword      MATERIAL rgb amb emis spec shi trans



" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_ac3d_syn_inits")
  if version < 508
    let did_ac3d_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif
  HiLink ac3dMatKeyword        Statement
  HiLink ac3dStringS           String
  HiLink ac3dStringD           String
  HiLink ac3dIdentifier        Identifier
  HiLink ac3dObject            Identifier

  HiLink ac3dFunction          Function
  HiLink ac3dComment           Comment
  HiLink ac3dSpecial           Special
  HiLink ac3dCharacter         Character
  HiLink ac3dNumber            Number
  HiLink ac3dFloat             Float
  HiLink ac3dIdentifier        Identifier
  HiLink ac3dConditional       Conditional
  HiLink ac3dRepeat            Repeat
  HiLink ac3dOperator          Operator
  HiLink ac3dType              Type
  HiLink ac3dError             Error
  HiLink ac3dBoolean           Boolean
  delcommand HiLink
endif

let b:current_syntax = "ac3d"
if main_syntax == 'ac3d'
  unlet main_syntax
endif

" vim: ts=8

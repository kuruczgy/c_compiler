project('c_compiler', 'c')

incdir = include_directories('include')

ds_proj = subproject('ds')
ds_vec_dep = ds_proj.get_variable('ds_vec_dep')

lex = find_program('lex')
bison = find_program('bison')

lgen = generator(lex,
output : '@BASENAME@.yy.c',
arguments : ['-t', '@INPUT@'],
capture : true)

lfiles = lgen.process('src/c.l')

pgen = generator(bison,
output : ['@BASENAME@.tab.c', '@BASENAME@.tab.h'],
arguments : ['@INPUT@', '--defines=@OUTPUT1@', '--output=@OUTPUT0@'])

pfiles = pgen.process('src/c.y')

c_compiler = executable(
  'c_compiler',
  'src/ast.c',
  lfiles, pfiles,
  dependencies : [ ds_vec_dep ],
  include_directories : incdir
)

# a bit ugly because LaTeX needs to be compiled twice, but does the job...
prog_pdflatex = find_program('pdflatex', required: false)
if prog_pdflatex.found()
  foreach tex : [ 'Documentation/spec.tex' ]
    basename = tex.split('.')[-2].split('/')[-1]
    first_pass = custom_target(basename + '_first_pass',
      input : tex,
      output : [ basename + '.aux' ],
      command : [
	prog_pdflatex,
	'-output-directory=@OUTDIR@',
	'@INPUT@',
      ],
    )
    custom_target(basename,
      input : [ tex, first_pass ],
      output : basename + '.pdf',
      command : [
	prog_pdflatex,
	'-output-directory=@OUTDIR@',
	'@INPUT@',
      ],
      build_by_default: true,
    )
  endforeach
endif

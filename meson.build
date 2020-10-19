project('c_compiler', 'c')

executable(
  'c_compiler',
  'src/main.c',
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

project('c_compiler', 'c', default_options : [ 'c_std=c11' ])

incdir = include_directories('include')

ds_proj = subproject('ds')
ds_vec_dep = ds_proj.get_variable('ds_vec_dep')
ds_hashmap_dep = ds_proj.get_variable('ds_hashmap_dep')

lex = find_program('lex')
bison = find_program('bison')
nasm = find_program('nasm')
gcc = find_program('gcc')

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
  'src/cg.c',
  lfiles, pfiles,
  dependencies : [ ds_vec_dep, ds_hashmap_dep ],
  include_directories : incdir
)

comp_asm = generator(c_compiler,
  output : [ '@BASENAME@.s' ],
  arguments : [ 'asm', '@INPUT@' ],
  capture : true
)

# compilation tests
foreach item : [
  { 'c': 'test/bf_interp.c' },
  { 'c': 'test/pointers.c', 't': true },
  { 'c': 'test/scopes.c', 't': true },
]
  c_file = item.get('c')
  do_test = item.get('t', false)
  test_s = comp_asm.process(c_file)
  test_o = custom_target(
    c_file.underscorify() + '_o',
    input : test_s,
    output : [ '@BASENAME@.o' ],
    command : [ nasm, '-f', 'elf64', '-o', '@OUTPUT@', '@INPUT@' ]
  )
  test_exe = executable(
    c_file.underscorify() + '_exe',
    test_o,
    link_args: [ '-static' ],
  )
  if do_test
    test(
      c_file.underscorify() + '_test',
      test_exe,
      timeout: 2,
    )
  endif
endforeach

# parsing tests
foreach c_file : [
  'declarators.c',
]
  path = 'test/syntax/' + c_file
  test(
    path.underscorify(),
    c_compiler,
    args: [ 'ast', files(path) ],
    timeout: 2,
  )
endforeach

# compilation error tests
foreach c_file : [
  'inc_lvalue.c',
  'addressof_lvalue.c',
  'add_ptr.c',
  'add_non_arith.c',
  'sub_ptr.c',
]
  path = 'test/error/' + c_file
  test(
    path.underscorify(),
    c_compiler,
    args: [ 'asm', files(path) ],
    timeout: 2,
    should_fail: true
  )
endforeach

# a bit ugly because LaTeX needs to be compiled twice, but does the job...
prog_pdflatex = find_program('pdflatex', required: false)
if prog_pdflatex.found()
  foreach tex : [ 'Documentation/spec.tex', 'Documentation/paper.tex' ]
    basename = tex.split('.')[-2].split('/')[-1]
    tagged = vcs_tag(
      command : [ 'git', 'rev-parse', '--short', 'HEAD' ],
      input: tex,
      output: basename + '.tex'
    )
    first_pass = custom_target(basename + '_first_pass',
      input : tagged,
      output : [ basename + '.aux' ],
      command : [
	prog_pdflatex,
	'-output-directory=@OUTDIR@',
	'@INPUT@',
      ],
    )
    custom_target(basename,
      input : [ tagged, first_pass ],
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

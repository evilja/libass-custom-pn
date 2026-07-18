test_src = files('test.c')

libass_test = executable(
    'libass_test',
    test_src,
    install: false,
    include_directories: incs,
    dependencies: deps + png_deps,
    link_with: libass_for_tools,
)

metrics_test = executable(
    'metrics_test',
    files('metrics.c'),
    install: false,
    include_directories: incs,
    dependencies: deps,
    link_with: libass_for_tools,
    c_args: '-DTEST_FONT_PATH="@0@"'.format(
        meson.project_source_root() / 'compare/test/font1.ttf',
    ),
)

test('metrics', metrics_test)

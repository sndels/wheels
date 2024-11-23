pushd "%~dp0.."

for %%b in (debug, debugoptimized, release) do (
    if not exist "build/%%b" (
        meson setup build/%%b --backend vs --buildtype %%b
    )
)

popd

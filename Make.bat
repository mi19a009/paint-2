CD "%~DP0"
MSBuild /Target:Rebuild /Property:Configuration=Release;Platform=x64
EXIT /B %ERRORLEVEL%

#!/bin/bash

# Run this, then run "bash" from inside cmd.exe. They you will be able to call cl.exe from inside bash.
cmd.exe /k "cd C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build && vcvars64.bat && bash"

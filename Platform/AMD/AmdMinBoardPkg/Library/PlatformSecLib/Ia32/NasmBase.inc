;*****************************************************************************
; AMD Generic Encapsulated Software Architecture
;
; Copyright (C) 2008 - 2024 Advanced Micro Devices, Inc. All rights reserved.
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; $Workfile:: NasmBase.inc
;
;  $Revision$    $Date$
;
; Description:
;

%ifnmacro _if 1-3
%macro _if 1-3

    nop  ;WA for nasm to avoid label incorrect issue

    %push _if

    %if %0 = 1

        %ifidn %1, carry
            jae  %$ifnot
        %elifidn %1, ncarry
            jb   %$ifnot
        %elifidn %1, zero
            jne  %$ifnot
        %elifidn %1, nzero
            je   %$ifnot
        %else
            j%-1 %$ifnot
        %endif

    %endif

    %if %0 = 2
        %error  "2 parameters is not allowed"
    %endif

    %if %0 = 3
        cmp   %1, %3
        j%-2  %$ifnot
    %endif

%endmacro
%endif


%ifnmacro _else 0
%macro _else 0

    %ifctx _if
        %repl   _else
        jmp     %$ifend
        %$ifnot:
    %else
        %error  "expected '_if' before '_else'"
    %endif

%endmacro
%endif


%ifnmacro _endif 0
%macro _endif 0

    %ifctx _if
        %$ifnot:
    %elifctx _else
        %$ifend:
    %else
        %error  "expected '_if' or '_else' before '_endif'"
    %endif

    %pop

%endmacro
%endif


%ifnmacro _while 1-3
%macro _while 1-3

    nop  ;WA for nasm to avoid label incorrect issue

    %push _while


    %if %0 = 1
        %define %$arg1 %1
        %define %$arg2 one_parameter
        %define %$arg3 one_parameter
    %endif

    %if %0 = 2
        %error  "2 parameters is not allowed"
    %endif

    %if %0 = 3
        %define %$arg1 %1
        %define %$arg2 %2
        %define %$arg3 %3
    %endif

    jmp %$whilecheck
    %$whilestart:
%endmacro
%endif


%ifnmacro _endw 0
%macro _endw 0

    %ifctx _while
        %$whilecheck:
        %ifidn %$arg2, one_parameter
            %ifidn %$arg1, carry
                jb   %$whilestart
            %elifidn %$arg1, ncarry
                jae  %$whilestart
            %elifidn %$arg1, zero
                je   %$whilestart
            %elifidn %$arg1, nzero
                jne  %$whilestart
            %else
                j%1  %$whilestart
            %endif
        %else
            cmp %$arg1, %$arg3
            j%$arg2 %$whilestart
        %endif
    %else
        %error  "expected '_while' before '_endw'"
    %endif

    %pop

%endmacro
%endif


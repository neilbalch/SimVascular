/* Copyright (c) Stanford University, The Regents of the University of
 *               California, and others.
 *
 * All Rights Reserved.
 *
 * See Copyright-SimVascular.txt for additional details.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "sv3_PyUtil.h"
#include <string>

//--------------------------
// Sv3PyUtilGetFunctionName
//--------------------------
// Get the function name used to display error messages for the Python API.
//
// Module functions are prefixed with '<MODULE_NAME>_' so replaced the '_'
// with a '.' to make the name look at it would if referenced from Python.
//
std::string Sv3PyUtilGetFunctionName(const char* functionName)
{
    std::string name(functionName);
    std::size_t pos = name.find("_");
    if (pos == std::string::npos) {
        return name;
    }
    return name.replace(pos, 1, ".");
}

//-----------------------
// Sv3PyUtilGetMsgPrefix 
//-----------------------
// Get the string used to prefix an error message for the Python API. 
//
// When an error occurs in the API PyErr_SetString() is called with an error message and 
// the enclosing function returns NULL. The Python API does not automatically print the 
// function name where an exception occurs so add it to the string passed to PyErr_SetString()
// using the prefix created here.
//
std::string Sv3PyUtilGetMsgPrefix(const std::string& functionName)
{
    std::string msgp = functionName + "() "; 
    return msgp;
}





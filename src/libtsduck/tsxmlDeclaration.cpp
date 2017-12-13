//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsxmlDeclaration.h"
#include "tsxmlDocument.h"
#include "tsxmlParser.h"
TSDUCK_SOURCE;

// Default XML declaration.
const ts::UChar* const ts::xml::Declaration::DEFAULT_XML_DECLARATION = u"xml version='1.0' encoding='UTF-8'";


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::xml::Declaration::Declaration(Report& report, size_t line) :
    Node(report, line)
{
}

ts::xml::Declaration::Declaration(Document* parent, const UString& value) :
    Node(parent, value.empty() ? DEFAULT_XML_DECLARATION : value)
{
}


//----------------------------------------------------------------------------
// Parse the node.
//----------------------------------------------------------------------------

bool ts::xml::Declaration::parseNode(Parser& parser, const Node* parent)
{
    // The current point of parsing is right after "<?".
    // The content of the declaration is up (but not including) the "?>".

    bool ok = parser.parseText(_value, u"?>", true, false);
    if (!ok) {
        _report.error(u"line %d: error parsing XML declaration, not properly terminated", {lineNumber()});
    }
    else if (dynamic_cast<const Document*>(parent) == 0) {
        _report.error(u"line %d: misplaced declaration, not directly inside a document", {lineNumber()});
        ok = false;
    }
    return ok;
}

#pragma once
#ifndef CATA_SRC_SCP_294_H
#define CATA_SRC_SCP_294_H

#include <string>
#include <vector>

#include "type_id.h"

class JsonObject;

// A scripted SCP-294 request.  Each entry maps one or more phrases to a fixed
// item (or to a refusal), and is checked before the generic liquid resolver
// runs, so the article's gags and their aliases take priority over a literal
// name match.
struct scp_294_request {
    std::string id;                  // purely informational
    std::vector<std::string> match;  // phrases, stored lower-cased and trimmed
    itype_id item = itype_id::NULL_ID();  // what to dispense
    std::string response;            // if non-empty, dispense nothing instead
    std::string message;             // optional flavour shown after resolving
};

// Data loading / lifecycle, wired in init.cpp.
void load_scp_294_request( const JsonObject &jo, const std::string &src );
void reset_scp_294_request();
// Validates the loaded table; runs in the consistency-check phase.
void check_scp_294_requests();

// The loaded special-request table.
const std::vector<scp_294_request> &scp_294_requests();

// Resolve a player's typed request to an item id to dispense.  Returns a null id
// when the request should be refused (OUT OF RANGE).  `out_message` receives any
// optional flavour text for the request.
itype_id scp_294_resolve( const std::string &request, std::string &out_message );

#endif // CATA_SRC_SCP_294_H

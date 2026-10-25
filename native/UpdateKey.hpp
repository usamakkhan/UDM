#pragma once
// The public half of the update signing key, written by tools/update-key.ps1 -Create.
// All zeros means no key is built in: UDM then verifies no manifest and offers no update,
// and Help > Check for Update falls back to the release page.
namespace udm {
inline constexpr unsigned char updatePublicKey[64]={0};
inline constexpr const char* updateKeyId="";
}

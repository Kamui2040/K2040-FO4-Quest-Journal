#include "JournalGameplayChord.h"
#include <cassert>
using K2040::JournalGameplayChord::Packed;
using K2040::JournalGameplayChord::Valid;
int main(){
    assert(Packed("Mouse4")==259u);
    assert(Packed("Mouse5")==260u);
    assert(Packed("Ctrl+Mouse5")==(260u|(2u<<9)));
    assert(Packed("Shift+G")==('G'|(1u<<9)));
    assert(Packed("F12")==123u);
    assert(Valid("Unbound")&&Packed("Unbound")==0);
    for(const auto* key:{"F3","Mouse6","Ctrl+Ctrl+Mouse4","Shift+Ctrl+Mouse4",
          "Alt+Shift+G","Ctrl+","Shift+F13","Ctrl+F0","G+Mouse5"})
        assert(!Valid(key));
    assert(Valid("Ctrl+Shift+B"));
}

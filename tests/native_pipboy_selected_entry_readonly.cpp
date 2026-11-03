#include "../research/native_pipboy_selection/ReadOnlySelectedEntry.h"
#include "../src/QuestTrackingPolicy.h"
#include <cassert>
#include <cstdint>
#include <map>
#include <memory>
#include <string>

namespace P = K2040::PipboySelectedEntryResearch;
struct Node {
    enum class Type {kMissing,kObject,kInteger,kUInt,kBoolean} type{Type::kMissing};
    std::uint32_t number{};
    std::map<std::string,std::shared_ptr<Node>> fields;
};
struct Value {
    std::shared_ptr<Node> node{};
    bool IsObject()const{return node&&node->type==Node::Type::kObject;}
    bool IsDisplayObject()const{return false;}
    bool IsArray()const{return false;}
    bool IsUInt()const{return node&&node->type==Node::Type::kUInt;}
    bool IsBoolean()const{return node&&node->type==Node::Type::kBoolean;}
    bool GetBoolean()const{return node&&node->number!=0;}
    bool IsInt()const{return node&&node->type==Node::Type::kInteger;}
    std::uint32_t GetUInt()const{return node?node->number:0u;}
    int GetInt()const{return node?static_cast<int>(node->number):0;}
    bool GetMember(const char* key,Value* output)const{
        if(!IsObject())return false;
        auto it=node->fields.find(key);
        if(it==node->fields.end())return false;
        output->node=it->second;return true;
    }
};
struct Movie {
    Value root;
    bool GetVariable(Value* output,const char* path)const{
        if(std::string(path)!="root.Menu_mc"||!root.node)return false;
        *output=root;return true;
    }
};
std::shared_ptr<Node> object(){
    auto x=std::make_shared<Node>();x->type=Node::Type::kObject;return x;
}
void number(const std::shared_ptr<Node>& object,const char* key,
            std::uint32_t n,Node::Type type=Node::Type::kUInt){
    auto v=std::make_shared<Node>();v->type=type;v->number=n;
    object->fields[key]=v;
}
int main(){
    auto menu=object(),page=object(),tab=object(),ql=object();
    auto root=object(),ol=object(),nested=object();
    menu->fields["CurrentPage"]=page;
    page->fields["QuestsTab_mc"]=tab;
    tab->fields["QuestsList_mc"]=ql;
    number(tab,"visible",1,Node::Type::kBoolean);
    tab->fields["ObjectivesList_mc"]=ol;
    ql->fields["selectedEntry"]=root;
    ol->fields["selectedEntry"]=nested;
    Movie m{Value{menu}};
    auto check=[](const P::Observation& observation){
        assert(!observation.nativeCommandTripleVerified);
        assert(!observation.nativeWriteAuthorized);
    };
    assert((P::Inspect<Movie,Value>(Movie{}).status==P::Status::kMenuUnavailable));
    auto broken=*menu;menu->fields.erase("CurrentPage");
    assert((P::Inspect<Movie,Value>(m).status==P::Status::kPageUnavailable));
    *menu=broken;
    number(tab,"visible",0,Node::Type::kBoolean);
    assert((P::Inspect<Movie,Value>(m).status==P::Status::kTabUnavailable));
    number(tab,"visible",1,Node::Type::kBoolean);
    number(root,"formID",0xFE00ABCD); // dynamic mod-type FormID, never whitelisted
    auto ordinary=P::Inspect<Movie,Value>(m);
    assert(ordinary.status==P::Status::kOrdinaryQuestSelection);
    check(ordinary);
    number(root,"formID",0); // vanilla synthetic Misc navigation
    auto unselected=*ol;ol->fields.erase("selectedEntry");
    auto missing=P::Inspect<Movie,Value>(m);
    assert(missing.status==P::Status::kMiscChildNotSelected);
    check(missing);
    *ol=unselected;
    number(nested,"formID",0xFE00ABCD);
    number(nested,"instance",9);
    number(nested,"type",6);
    auto misc=P::Inspect<Movie,Value>(m);
    assert(misc.status==P::Status::kMiscChildSelected);
    assert(misc.syntheticMiscParent && misc.selectedFromNativeObjectivesList);
    assert(misc.nestedFormIDTyped && misc.nestedInstanceTyped &&
           misc.nestedTypeTyped && !misc.nestedQuestTypeTyped);
    check(misc);
    nested->fields.erase("type");
    misc=P::Inspect<Movie,Value>(m);
    assert(!misc.nestedTypeTyped);
    check(misc);
    number(root,"formID",static_cast<std::uint32_t>(-1),Node::Type::kInteger);
    assert((P::Inspect<Movie,Value>(m).status==P::Status::kRootIdentityUnavailable));
    static_assert(!K2040::QuestTracking::CanWritePipboy());
}

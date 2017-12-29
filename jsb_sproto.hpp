#ifndef JSB_SPROTO_H
#define JSB_SPROTO_H
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WINRT || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)

#include "cocos/scripting/js-bindings/jswrapper/SeApi.h"

extern se::Object* __jsb_sproto_object;
extern se::Class* __jsb_sproto_class;



bool register_all_sproto(se::Object* obj);

//		{ "newproto", lnewproto },
//		{ "deleteproto", ldeleteproto },
//		{ "dumpproto", ldumpproto },
//		{ "querytype", lquerytype },
//		{ "decode", ldecode },
//		{ "protocol", lprotocol },
//		{ "loadproto", lloadproto },
//		{ "saveproto", lsaveproto },
//		{ "default", ldefault },
//		{ NULL, NULL },


#endif

#endif

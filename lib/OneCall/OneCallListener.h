#pragma once
#include "OneCallData.h"
#include <JsonStreamingParser2.h>

class OneCallListener: public JsonHandler {
    private:
        OneCallData *data;
    public:
        OneCallListener(OneCallData *data);
        void endArray(ElementPath path);
        void endDocument();
        void endObject(ElementPath path);
        void startArray(ElementPath path);
        void startDocument();
        void startObject(ElementPath path);
        void value(ElementPath path, ElementValue value);
        void whitespace(char c);
};
/*
 * Copyright 2015 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkDocument.h"
#include "SkXPSDevice.h"
#include "SkStream.h"

class SkDocument_XPS : public SkDocument {
public:
    SkDocument_XPS(SkWStream* stream,
                   void (*doneProc)(SkWStream*, bool),
                   SkScalar dpi)
        : SkDocument(stream, doneProc) {
        const SkScalar kPointsPerMeter = SkDoubleToScalar(360000.0 / 127.0);
        fUnitsPerMeter.set(kPointsPerMeter, kPointsPerMeter);
        SkScalar pixelsPerMeterScale = SkDoubleToScalar(dpi * 5000.0 / 127.0);
        fPixelsPerMeter.set(pixelsPerMeterScale, pixelsPerMeterScale);
        fDevice.beginPortfolio(stream);
    }

    virtual ~SkDocument_XPS() {
        // subclasses must call close() in their destructors
        this->close();
    }

protected:
    virtual SkCanvas* onBeginPage(SkScalar width,
                                  SkScalar height,
                                  const SkRect& trimBox) SK_OVERRIDE {
        fDevice.beginSheet(fUnitsPerMeter, fPixelsPerMeter,
                           SkSize::Make(width, height));
        fCanvas.reset(SkNEW_ARGS(SkCanvas, (&fDevice)));
        fCanvas->clipRect(trimBox);
        fCanvas->translate(trimBox.x(), trimBox.y());
        return fCanvas.get();
    }

    void onEndPage() SK_OVERRIDE {
        SkASSERT(fCanvas.get());
        fCanvas->flush();
        fCanvas.reset(NULL);
        fDevice.endSheet();
    }

    bool onClose(SkWStream*) SK_OVERRIDE {
        SkASSERT(!fCanvas.get());
        return fDevice.endPortfolio();
    }

    void onAbort() SK_OVERRIDE {}

private:
    SkXPSDevice fDevice;
    SkAutoTUnref<SkCanvas> fCanvas;
    SkVector fUnitsPerMeter;
    SkVector fPixelsPerMeter;
};

///////////////////////////////////////////////////////////////////////////////

SkDocument* SkDocument::CreateXPS(SkWStream* stream, SkScalar dpi) {
    return stream ? SkNEW_ARGS(SkDocument_XPS, (stream, NULL, dpi)) : NULL;
}

static void delete_wstream(SkWStream* stream, bool aborted) {
    SkDELETE(stream);
}

SkDocument* SkDocument::CreateXPS(const char path[], SkScalar dpi) {
    SkAutoTDelete<SkFILEWStream> stream(SkNEW_ARGS(SkFILEWStream, (path)));
    if (!stream->isValid()) {
        return NULL;
    }
    return SkNEW_ARGS(SkDocument_XPS, (stream.detach(), delete_wstream, dpi));
}

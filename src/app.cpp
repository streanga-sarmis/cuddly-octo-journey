#include <SDL2/SDL.h>

#include "gl.h"
#include "gl_utilities.h"
#include "render_buffer.h"
#include "math.h"
#include "shader.h"
#include "font.h"

#include "gap_buffer.h"
#include "window.h"
#include "panel.h"
#include "panel_actions.h"

#include <time.h>

#define TURBO_TIME 1

u32 windowWidth = 1280;
u32 windowHeight = 768;

u32 windowCount = 2;
EditorWindow windows[2] = {
    windowCreate(windowWidth / 2, windowHeight, 0, 0),
    windowCreate(windowWidth / 2, windowHeight, windowWidth / 2, 0)
};

u32 currentWindowIndex = 0;
EditorWindow* currentWindow = &windows[currentWindowIndex];
GapBuffer* currentBuffer = &currentWindow->buffer;

int main(int argumentCount, char* arguments[]){
#if 1
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0){
        TRACE("Could not initialize SDL2\n");
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_Window *window = SDL_CreateWindow("App",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          windowWidth, windowHeight,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext glcontext = SDL_GL_CreateContext(window);

    initializeGL();

    TRACE("%s\n", glGetString(GL_VERSION));

    SDL_Event event;

    RenderBuffer renderBuffer = createVertexArrayObject();
    pushPreProcesedQuadsIndices(&renderBuffer);

    RenderBuffer renderBufferBackground = createVertexArrayObject();
    RenderBuffer renderBufferUI = createVertexArrayObject();

    FontGL font = createFont();
    
    Shader shader = createShader();
    Shader shaderUI = createShaderCursor();

    SHADER_SCOPE(shader.programId, {
        shaderSetUniform4m(shader.locations.matrixPerspective, 
                           orthographic(0, windowWidth, 0, windowHeight));
    });

    SHADER_SCOPE(shaderUI.programId, {
        shaderSetUniform4m(shaderUI.locations.matrixPerspective, 
                           orthographic(0, windowWidth, 0, windowHeight));
    });

    bool done = false;

    bool backspaceReleased = true;
    bool homeReleased = true;
    bool endReleased = true;
    bool arrowReleased[4] = {true, true, true, true};
    bool tabReleased = true;
    bool newlineReleased = true;
    bool pageDownReleased = true;
    bool pageUpReleased = true;

    bool controlSeeking = false;

    bool backspaceTurbo = false;
    bool arrowTurbo = false;

    u32 backspaceTime = 0;
    u32 arrowTime = 0;
#endif

    // GapBuffer buffer = {};
    // buffer.data = new u8[GAP_DEFAULT_SIZE];
    // memset(buffer.data, 0, GAP_DEFAULT_SIZE);
    // buffer.size = GAP_DEFAULT_SIZE;
    // buffer.cursor = 0;
    // buffer.gap.start = 0;
    // buffer.gap.end = GAP_DEFAULT_SIZE;

    if(argumentCount > 1){
        currentWindow->buffer = gapReadFile(arguments[1]);
    } else {
        currentWindow->buffer = gapCreateEmpty();
    }

    Panel openFilePanel = panelCreate({0, 0, 0}, {400, FONT_HEIGHT * 3 + 12 + 4}, "Open file");
    openFilePanel.action = openFileAction;

    Panel findPanel = panelCreate({0, 0, 0}, {400, FONT_HEIGHT * 3 + 12 + 4}, "Find");

    Panel gotoLinePanel = panelCreate({0, 0, 0}, {400, FONT_HEIGHT * 3 + 12 + 4}, "Goto line");

    Panel saveFilePanel = panelCreate({0, 0, 0}, {400, FONT_HEIGHT * 3 + 12 + 4}, "Save file");
    saveFilePanel.action = saveFileAction;

    Panel panel = openFilePanel;

    bool panelActive = false;

#if 1
    bool shiftPressed = false;

    char shiftCharactersLUT[256];
    for(char c = 'a'; c <= 'z'; ++c){
        shiftCharactersLUT[c] = c - ' ';
    }

    shiftCharactersLUT['1'] = '!';
    shiftCharactersLUT['2'] = '@';
    shiftCharactersLUT['3'] = '#';
    shiftCharactersLUT['4'] = '$';
    shiftCharactersLUT['5'] = '%';
    shiftCharactersLUT['6'] = '^';
    shiftCharactersLUT['7'] = '&';
    shiftCharactersLUT['8'] = '*';
    shiftCharactersLUT['9'] = '(';
    shiftCharactersLUT['0'] = ')';
    shiftCharactersLUT['['] = '{';
    shiftCharactersLUT[']'] = '}';
    shiftCharactersLUT['\''] = '"';
    shiftCharactersLUT['\\'] = '|';
    shiftCharactersLUT['/'] = '?';
    shiftCharactersLUT[','] = '<';
    shiftCharactersLUT['.'] = '>';
    shiftCharactersLUT['='] = '+';
    shiftCharactersLUT['-'] = '_';

    i32 time = 0;

    while(!done){
        time++;
        time %= 20;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        while(SDL_PollEvent(&event)){
            switch(event.type){
                case SDL_WINDOWEVENT: {
                        switch(event.window.event){
                            case SDL_WINDOWEVENT_SIZE_CHANGED: {
                                    u32 newWidth = event.window.data1;
                                    u32 newHeight = event.window.data2;
                                    // TODO(Sarmis) I have to fix these 
                                    // depending on the new size,
                                    // the already existing offset needs to 
                                    // update

                                    for(int i = 0; i < windowCount; ++i){
                                        r32 wr = (r32)windows[i].width / (r32)windowWidth;
                                        r32 hr = (r32)windows[i].height / (r32)windowHeight;

                                        windows[i].width = wr * newWidth;
                                        windows[i].height = hr * newHeight;

                                        windows[i].top = hr * windows[i].top;
                                        windows[i].bottom = windows[i].top + windows[i].height;

                                        windows[i].left = ((r32)windows[i].left / (r32)windowWidth) * newWidth;

                                        windows[i].scrollTop = windows[i].top;
                                        windows[i].scrollBottom = windows[i].bottom;
                                        windows[i].buffer.cursor = 0;
                                        windows[i].scrollY = 0;
                                        windows[i].scrollX = 0;
                                    }

                                    SHADER_SCOPE(shader.programId, {
                                        shaderSetUniform4m(shader.locations.matrixPerspective, 
                                                        orthographic(0, newWidth, 0, newHeight));
                                    });

                                    SHADER_SCOPE(shaderUI.programId, {
                                        shaderSetUniform4m(shaderUI.locations.matrixPerspective, 
                                                        orthographic(0, newWidth, 0, newHeight));
                                    });

                                    windowWidth = newWidth;
                                    windowHeight = newHeight;
                                }
                                break;
                        }
                    }
                    break;

                case SDL_KEYUP: {
                        switch (event.key.keysym.scancode) {
                            case SDL_SCANCODE_LCTRL: case SDL_SCANCODE_RCTRL: {
                                    controlSeeking = false;
                                }
                                break;

                            case SDL_SCANCODE_PAGEUP: {
                                    pageUpReleased = true;
                                }
                                break;
    
                            case SDL_SCANCODE_PAGEDOWN: {
                                    pageDownReleased = true;
                                }
                                break;

                            case SDL_SCANCODE_HOME: {
                                    homeReleased = true;
                                }
                                break;

                            case SDL_SCANCODE_END: {
                                    endReleased = true;
                                }
                                break;

                            case SDL_SCANCODE_LSHIFT: case SDL_SCANCODE_RSHIFT: {
                                    shiftPressed = false;
                                }
                                break;

                            case SDL_SCANCODE_TAB: {
                                    tabReleased = true;
                                }
                                break;

                            case SDL_SCANCODE_LEFT:{
                                    arrowReleased[3] = true;
                                    arrowTurbo = false;
                                    arrowTime = 0;
                                } 
                                break;

                            case SDL_SCANCODE_RIGHT: {
                                    arrowReleased[2] = true;
                                    arrowTurbo = false;
                                    arrowTime = 0;
                                }
                                break;

                            case SDL_SCANCODE_UP:{
                                    arrowReleased[1] = true;
                                    arrowTurbo = false;
                                    arrowTime = 0;
                                }
                            
                            case SDL_SCANCODE_DOWN: {
                                    arrowReleased[0] = true;
                                    arrowTurbo = false;
                                    arrowTime = 0;
                                }
                                break;

                            case SDL_SCANCODE_DELETE: case SDL_SCANCODE_BACKSPACE: {
                                    backspaceReleased = true;
                                    backspaceTurbo = false;
                                    backspaceTime = 0;
                                }
                                break;

                            case SDL_SCANCODE_RETURN2: case SDL_SCANCODE_RETURN: {
                                    newlineReleased = true;
                                }
                                break;
                        }
                    }
                    break;

                case SDL_KEYDOWN: {
                        switch (event.key.keysym.scancode) {
                            case SDL_SCANCODE_ESCAPE: {
                                    if(panelActive){
                                        panelActive = false;
                                    }
                                }
                                break;

                            case SDL_SCANCODE_LCTRL: case SDL_SCANCODE_RCTRL: {
                                    controlSeeking = true;
                                }
                                break;

                            case SDL_SCANCODE_LSHIFT: case SDL_SCANCODE_RSHIFT: {
                                    shiftPressed = true;
                                }
                                break;

                            case SDL_SCANCODE_PAGEUP: {
                                    if(pageUpReleased){
                                        u32 heightInLines = windowHeight / FONT_HEIGHT;
                                        gapSeekCursorINewlinesIfPossible(currentBuffer, -heightInLines);
                                    }
                                    pageUpReleased = false;
                                }
                                break;

                            case SDL_SCANCODE_PAGEDOWN: {
                                    // SPONGE 
                                    // bug with the pagedown when resizing
                                    // after scroll was done
                                    if(pageDownReleased){
                                        u32 heightInLines = windowHeight / FONT_HEIGHT;
                                        gapSeekCursorINewlinesIfPossible(currentBuffer, heightInLines);
                                    }
                                    pageDownReleased = false;
                                }
                                break;

                            case SDL_SCANCODE_HOME: {
                                    if(homeReleased){
                                        if(shiftPressed){
                                            if(!gapGetSelectionSize(currentBuffer)){
                                                // there is no current selection alive
                                                currentBuffer->selection.end = currentBuffer->cursor;
                                            }
                                        } else {
                                            currentBuffer->selection.start = currentBuffer->selection.end;
                                        }

                                        gapSeekCursorToPreviousNewline(currentBuffer);

                                        if(currentBuffer->data[UserToGap(currentBuffer->gap, currentBuffer->cursor)] == '\n'){
                                            gapIncreaseCursor(currentBuffer);
                                        }

                                        if(shiftPressed){
                                            currentBuffer->selection.start = currentBuffer->cursor;
                                        }
                                    }
                                    homeReleased = false;
                                }
                                break;

                            case SDL_SCANCODE_END: {
                                    // TODO(Sarmis) this could end at a weird 
                                    // location at the end of the buffer
                                    // add bound checking
                                    if(endReleased){
                                        if(shiftPressed){
                                            if(!gapGetSelectionSize(currentBuffer)){
                                                // there is no current selection alive
                                                currentBuffer->selection.start = currentBuffer->cursor;
                                            }
                                        } else {
                                            currentBuffer->selection.start = currentBuffer->selection.end;
                                        }

                                        if(currentBuffer->data[UserToGap(currentBuffer->gap, currentBuffer->cursor)] != '\n'){
                                            gapSeekCursorToNewline(currentBuffer);
                                        }

                                        if(shiftPressed){
                                            currentBuffer->selection.end = currentBuffer->cursor;
                                        }
                                    }
                                    endReleased = false;
                                }
                                break;

                            case SDL_SCANCODE_F12: {
                                }
                                break;

                            case SDL_SCANCODE_TAB: {
                                    u8 tab = '\t';
                                    if(tabReleased){
                                        if(controlSeeking){
                                            ++currentWindowIndex;
                                            currentWindowIndex %= windowCount;
                                            currentWindow = &windows[currentWindowIndex];
                                            if(!panelActive){
                                                currentBuffer = &currentWindow->buffer;
                                            }
                                        } else {
                                            gapInsertCharacterAt(currentBuffer, tab, currentBuffer->cursor);
                                            gapIncreaseCursor(currentBuffer);
                                        }
                                    }
                                    tabReleased = false;
                                }
                                break;

                            case SDL_SCANCODE_RETURN2: case SDL_SCANCODE_RETURN: {
                                    u8 newline = '\n';
                                    if(newlineReleased){
                                        if(panelActive){
                                            
                                            panelActive = false;
                                            panel.action(currentWindow, &panel.buffer);

                                            currentBuffer = &currentWindow->buffer;

                                        } else {
                                            i32 tabs = gapGetAmontOfTabsBeforeCursor(currentBuffer);

                                            gapInsertCharacterAt(currentBuffer, newline, currentBuffer->cursor);
                                            gapIncreaseCursor(currentBuffer);

                                            while(tabs--){
                                                gapInsertCharacterAt(currentBuffer, '\t', currentBuffer->cursor);
                                                gapIncreaseCursor(currentBuffer);
                                            }
                                        }
                                    }
                                    newlineReleased = false;
                                }
                                break;

                            case SDL_SCANCODE_UP: {
                                    // TODO(Sarmis) this doesn't really work properly
                                    // did this quick to just have a basic thing ready
                                    // for this thing...
                                    if(arrowReleased[1] || arrowTurbo){
                                        i32 distance = gapGetDistanceFromPreviousNewline(currentBuffer);

                                        if(shiftPressed){
                                            if(!gapGetSelectionSize(currentBuffer)){
                                                // there is no current selection alive
                                                currentBuffer->selection.end = currentBuffer->cursor;
                                            }
                                        } else {
                                            currentBuffer->selection.start = currentBuffer->selection.end;
                                        }

                                        if(controlSeeking){
                                            gapSeekCursorINewlinesIfPossible(currentBuffer, -10);
                                        } else {
                                            gapSeekCursorToPreviousNewline(currentBuffer);
                                            if(currentBuffer->cursor - 1 >= 0){
                                                i32 distanceOnPreviousLine = gapGetDistanceFromPreviousNewline(currentBuffer);
                                                distanceOnPreviousLine -= distance;
                                                if(distanceOnPreviousLine >= 0){
                                                    gapSeekCursor(currentBuffer, -distanceOnPreviousLine);
                                                }
                                            }
                                        }

                                        if(shiftPressed){
                                            currentBuffer->selection.start = currentBuffer->cursor;
                                        }
                                    } else {
                                        arrowTime++;
                                        if (arrowTime >= TURBO_TIME){
                                            arrowTurbo = true;
                                        }
                                    }
                                    arrowReleased[1] = false;
                                }
                                break;

                            case SDL_SCANCODE_DOWN: {
                                    if(arrowReleased[0] || arrowTurbo){
                                        i32 distance = gapGetDistanceFromPreviousNewline(currentBuffer);

                                        if(shiftPressed){
                                            if(!gapGetSelectionSize(currentBuffer)){
                                                // there is no current selection alive
                                                currentBuffer->selection.start = currentBuffer->cursor;
                                            }
                                        } else {
                                            currentBuffer->selection.start = currentBuffer->selection.end;
                                        }

                                        if(controlSeeking){
                                            gapSeekCursorINewlinesIfPossible(currentBuffer, 10);
                                        } else {
                                            gapSeekCursorToNewline(currentBuffer);

                                            if(currentBuffer->cursor + 1 < currentBuffer->size){
                                                gapIncreaseCursor(currentBuffer);
                                                i32 lineLength = gapGetDistanceToNewline(currentBuffer);
                                                if(distance <= lineLength){
                                                    gapSeekCursor(currentBuffer, distance - 1);
                                                } else {
                                                    gapSeekCursor(currentBuffer, lineLength);
                                                }
                                            }
                                        }

                                        if(shiftPressed){
                                            currentBuffer->selection.end = currentBuffer->cursor;
                                        }
                                    } else {
                                        arrowTime++;
                                        if (arrowTime >= TURBO_TIME){
                                            arrowTurbo = true;
                                        }
                                    }
                                    arrowReleased[0] = false;
                                }
                                break;

                            case SDL_SCANCODE_LEFT: {
                                    if(arrowReleased[3] || arrowTurbo){
                                        if(shiftPressed){
                                            if(!gapGetSelectionSize(currentBuffer)){
                                                // there is no current selection alive
                                                currentBuffer->selection.end = currentBuffer->cursor;
                                            }
                                        } else {
                                            currentBuffer->selection.start = currentBuffer->selection.end;
                                        }

                                        if(controlSeeking){
                                            gapSeekCursorToPreviousCapitalOrSpace(currentBuffer);
                                            if(currentBuffer->data[UserToGap(currentBuffer->gap, currentBuffer->cursor)] == '\n'){
                                                gapIncreaseCursor(currentBuffer);
                                            }
                                        } else {
                                            gapDecreaseCursor(currentBuffer);
                                        }

                                        if(shiftPressed){
                                            currentBuffer->selection.start = currentBuffer->cursor;
                                        }
                                    } else {
                                        arrowTime++;
                                        if (arrowTime >= TURBO_TIME){
                                            arrowTurbo = true;
                                        }
                                    }
                                    arrowReleased[3] = false;
                                }
                                break;

                            case SDL_SCANCODE_RIGHT: {
                                    if(arrowReleased[2] || arrowTurbo){
                                        if(shiftPressed){
                                            if(!gapGetSelectionSize(currentBuffer)){
                                                // there is no current selection alive
                                                currentBuffer->selection.start = currentBuffer->cursor;
                                            }
                                        } else {
                                            currentBuffer->selection.start = currentBuffer->selection.end;
                                        }

                                        if(controlSeeking){
                                            gapSeekCursorToCapitalOrSpace(currentBuffer);
                                        } else {
                                            gapIncreaseCursor(currentBuffer);
                                        }

                                        if(shiftPressed){
                                            currentBuffer->selection.end = currentBuffer->cursor;
                                        }
                                    } else {
                                        arrowTime++;
                                        if (arrowTime >= TURBO_TIME){
                                            arrowTurbo = true;
                                        }
                                    }
                                    arrowReleased[2] = false;
                                }
                                break;

                            case SDL_SCANCODE_DELETE: {
                                    if(backspaceReleased || backspaceTurbo){
                                        if(gapGetSelectionSize(currentBuffer)){
                                            gapRemoveCharactersInRange(currentBuffer, currentBuffer->selection.start, currentBuffer->selection.end);
                                            gapSeekCursor(currentBuffer, -(currentBuffer->selection.end - currentBuffer->selection.start));
                                        } else {
                                            gapRemoveCharacterNearAt(currentBuffer, currentBuffer->cursor);
                                        }
                                    } else {
                                        backspaceTime++;
                                        if (backspaceTime >= TURBO_TIME){
                                            backspaceTurbo = true;
                                        }
                                    }
                                    backspaceReleased = false;
                                }
                                break;


                            case SDL_SCANCODE_BACKSPACE: {
                                    // TODO(Sarmis) this results in segfault, needs bound checking
                                    if(backspaceReleased || backspaceTurbo){
                                        if(gapGetSelectionSize(currentBuffer)){
                                            gapRemoveCharactersInRange(currentBuffer, currentBuffer->selection.start, currentBuffer->selection.end);
                                            gapSeekCursor(currentBuffer, -(currentBuffer->selection.end - currentBuffer->selection.start));
                                        } else {
                                            if(!controlSeeking){
                                                gapRemoveCharacterAt(currentBuffer, currentBuffer->cursor);
                                                gapDecreaseCursor(currentBuffer);
                                            } else {
                                                u32 end = currentBuffer->cursor;
                                                gapSeekCursorToPreviousCapitalOrSpace(currentBuffer);
                                                u32 start = currentBuffer->cursor + 1;
                                                gapRemoveCharactersInRange(currentBuffer, start, end);
                                            }
                                        }
                                    } else {
                                        backspaceTime++;
                                        if (backspaceTime >= TURBO_TIME){
                                            backspaceTurbo = true;
                                        }
                                    }
                                    backspaceReleased = false;
                                }
                                break;
                                
                            default: {
                                    char character = event.key.keysym.sym;
                                    if(shiftPressed) {
                                        character = shiftCharactersLUT[character];
                                    }
                                    if(!controlSeeking){
                                        gapInsertCharacterAt(currentBuffer, character, currentBuffer->cursor);
                                        gapIncreaseCursor(currentBuffer);
                                    } else {
                                        switch (character) {
                                            case 'g':{
                                                    panelActive = true;
                                                    panel = gotoLinePanel;
                                                    panel.buffer = gapCreateEmpty();
                                                    panel.position.x = currentWindow->left;
                                                    panel.position.y = -panel.size.y;
                                                    currentBuffer = &panel.buffer;
                                                }
                                                break;
                                            case 'f': {
                                                    panelActive = true;
                                                    panel = findPanel;
                                                    panel.buffer = gapCreateEmpty();
                                                    panel.position.x = currentWindow->left;
                                                    panel.position.y = -panel.size.y;
                                                    currentBuffer = &panel.buffer;
                                                }
                                                break;

                                            case 'p': {
                                                    panelActive = true;
                                                    panel = openFilePanel;
                                                    panel.buffer = gapCreateEmpty();
                                                    panel.position.x = currentWindow->left;
                                                    panel.position.y = -panel.size.y;
                                                    currentBuffer = &panel.buffer;
                                                }
                                                break;

                                            case 's': {
                                                    if(currentBuffer->dirty){
                                                        if(!currentBuffer->filename){
                                                            // file is not on disk
                                                            // this is the first save ever
                                                            // for this file...
                                                            panelActive = true;
                                                            panel = saveFilePanel;
                                                            panel.buffer = gapCreateEmpty();
                                                            panel.position.x = currentWindow->left;
                                                            panel.position.y = -panel.size.y;
                                                            currentBuffer = &panel.buffer;
                                                            // clear previous panel gap buffer is curenly opened
                                                        } else {
                                                            gapWriteFile(currentBuffer);
                                                            currentWindow->backgroundColor = {0, 0.1, 0};
                                                            TRACE("Saved file %s\n", currentBuffer->filename);
                                                        }
                                                    }
                                                }
                                                break;

                                            case 'v': {
                                                    if(SDL_HasClipboardText()){
                                                        char* clipboard = SDL_GetClipboardText();
                                                        if(clipboard){
                                                            i32 distance = gapInsertNullTerminatedStringAt(currentBuffer, clipboard, currentBuffer->cursor);
                                                            gapSeekCursor(currentBuffer, distance);
                                                            SDL_free(clipboard);
                                                        }
                                                    }
                                                }
                                                break;

                                            case 'c': {
                                                    String copy = {};
                                                    // TODO(Sarmis) solve case in which 
                                                    // the gap in in the selection
                                                    if(currentBuffer->selection.end < currentBuffer->gap.start){
                                                        copy = subString(currentBuffer->bufferString, currentBuffer->selection.start, currentBuffer->selection.end);
                                                    } else {
                                                        copy = subString(currentBuffer->bufferString, UserToGap(currentBuffer->gap, currentBuffer->selection.start), UserToGap(currentBuffer->gap, currentBuffer->selection.end));
                                                    }
                                                    i32 status = SDL_SetClipboardText((char*)copy.data);
                                                    if(status){
                                                        printf("SDL_SetClipboardText failed: %s\n", SDL_GetError());
                                                    }
                                                }
                                                break;


                                            default:{
                                                }
                                                break;
                                        }
                                    }
                                    currentBuffer->selection.end = currentBuffer->selection.start;
                                }
                                break;
                        }
                    }
                    break;

                case SDL_QUIT: {
                        done = true;
                    }
                    break;
            }
        }


        v2 uvs[4] = {};

        glViewport(0, 0, windowWidth, windowHeight);

        glClearColor(DEFAULT_COLOR_BACKGROUND.x, DEFAULT_COLOR_BACKGROUND.y, DEFAULT_COLOR_BACKGROUND.z, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, font.textureId);
        
        currentWindow->backgroundColor = lerp(currentWindow->backgroundColor, DEFAULT_COLOR_BACKGROUND, 0.1);
        pushQuad(&renderBufferBackground, {currentWindow->left, currentWindow->top, 0},
                {currentWindow->width, currentWindow->height}, uvs, currentWindow->backgroundColor);

        for(int i = 0; i < windowCount; ++i){
            editorWindowRender(&windows[i], 
                        &shader, &shaderUI,
                        &renderBuffer, &renderBufferUI, &renderBufferBackground,
                        &font,
                        time, currentWindowIndex == i);
        }
        
        if(panelActive){
            panelRender(&panel, currentWindow,
                 &shader, &shaderUI,
                 &renderBuffer, &renderBufferUI, &renderBufferBackground,
                 &font,
                 time, windowWidth, windowHeight);
        }
        
        SDL_GL_SwapWindow(window);
    }
#endif
    return 0;
}



#include "Text.h"

#include <wincodec.h>
#include <iostream>
#include <fstream>
#include <string>

Font LoadFont(LPCWSTR filename, int windowWidth, int windowHeight)
{
    std::wifstream fs{};
    fs.open(filename);

    Font font;
    std::wstring tmp;
    int startpos;

    // extract font name
    fs >> tmp >> tmp; // info face="Arial"
    startpos = tmp.find(L"'") + 1;
    font.name = tmp.substr(startpos, tmp.size() - startpos - 1);

    // get font size
    fs >> tmp; // size=73
    startpos = tmp.find(L"=") + 1;
    font.size = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

    // bold, italic, charset, unicode, stretchH, smooth, aa, padding, spacing
    fs >> tmp >> tmp >> tmp >> tmp >> tmp >> tmp >> tmp; // bold=0 italic=0 charset="" unicode=0 stretchH=100 smooth=1 aa=1 

    // get padding
    fs >> tmp; // padding=5,5,5,5 
    startpos = tmp.find(L"=") + 1;
    tmp = tmp.substr(startpos, tmp.size() - startpos); // 5,5,5,5

    // get up padding
    startpos = tmp.find(L",") + 1;
    font.toppadding = std::stoi(tmp.substr(0, startpos)) / (float)windowWidth;

    // get right padding
    tmp = tmp.substr(startpos, tmp.size() - startpos);
    startpos = tmp.find(L",") + 1;
    font.rightpadding = std::stoi(tmp.substr(0, startpos)) / (float)windowWidth;

    // get down padding
    tmp = tmp.substr(startpos, tmp.size() - startpos);
    startpos = tmp.find(L",") + 1;
    font.bottompadding = std::stoi(tmp.substr(0, startpos)) / (float)windowWidth;

    // get left padding
    tmp = tmp.substr(startpos, tmp.size() - startpos);
    font.leftpadding = std::stoi(tmp) / (float)windowWidth;

    fs >> tmp; // spacing=0,0

    // get lineheight (how much to move down for each line), and normalize (between 0.0 and 1.0 based on size of font)
    fs >> tmp >> tmp; // common lineHeight=95
    startpos = tmp.find(L"=") + 1;
    font.lineHeight = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) / (float)windowHeight;

    // get base height (height of all characters), and normalize (between 0.0 and 1.0 based on size of font)
    fs >> tmp; // base=68
    startpos = tmp.find(L"=") + 1;
    font.baseHeight = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) / (float)windowHeight;

    // get texture width
    fs >> tmp; // scaleW=512
    startpos = tmp.find(L"=") + 1;
    font.textureWidth = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

    // get texture height
    fs >> tmp; // scaleH=512
    startpos = tmp.find(L"=") + 1;
    font.textureHeight = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

    // get pages, packed, page id
    fs >> tmp >> tmp; // pages=1 packed=0
    fs >> tmp >> tmp; // page id=0

    // get texture filename
    std::wstring wtmp;
    fs >> wtmp; // file="Arial.png"
    startpos = wtmp.find(L"'") + 1;
    font.fontImage = wtmp.substr(startpos, wtmp.size() - startpos - 1);

    // get number of characters
    fs >> tmp >> tmp; // chars count=97
    startpos = tmp.find(L"=") + 1;
    font.numCharacters = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

    // initialize the character list
    font.CharList = new FontChar[font.numCharacters];

    for (int c = 0; c < font.numCharacters; ++c)
    {
        // get unicode id
        fs >> tmp >> tmp; // char id=0
        startpos = tmp.find(L"=") + 1;
        font.CharList[c].id = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

        // get x
        fs >> tmp; // x=392
        startpos = tmp.find(L"=") + 1;
        font.CharList[c].u = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) / (float)font.textureWidth;

        // get y
        fs >> tmp; // y=340
        startpos = tmp.find(L"=") + 1;
        font.CharList[c].v = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) / (float)font.textureHeight;

        // get width
        fs >> tmp; // width=47
        startpos = tmp.find(L"=") + 1;
        tmp = tmp.substr(startpos, tmp.size() - startpos);
        font.CharList[c].width = (float)std::stoi(tmp) / (float)windowWidth;
        font.CharList[c].twidth = (float)std::stoi(tmp) / (float)font.textureWidth;

        // get height
        fs >> tmp; // height=57
        startpos = tmp.find(L"=") + 1;
        tmp = tmp.substr(startpos, tmp.size() - startpos);
        font.CharList[c].height = (float)std::stoi(tmp) / (float)windowHeight;
        font.CharList[c].theight = (float)std::stoi(tmp) / (float)font.textureHeight;

        // get xoffset
        fs >> tmp; // xoffset=-6
        startpos = tmp.find(L"=") + 1;
        font.CharList[c].xoffset = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) / (float)windowWidth;

        // get yoffset
        fs >> tmp; // yoffset=16
        startpos = tmp.find(L"=") + 1;
        font.CharList[c].yoffset = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) / (float)windowHeight;

        // get xadvance
        fs >> tmp; // xadvance=65
        startpos = tmp.find(L"=") + 1;
        font.CharList[c].xadvance = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) / (float)windowWidth;

        // get page
        // get channel
        fs >> tmp >> tmp; // page=0    chnl=0
    }

    // get number of kernings
    fs >> tmp >> tmp; // kernings count=96
    startpos = tmp.find(L"=") + 1;
    font.numKernings = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

    // initialize the kernings list
    font.KerningsList = new FontKerning[font.numKernings];

    for (int k = 0; k < font.numKernings; ++k)
    {
        // get first character
        fs >> tmp >> tmp; // kerning first=87
        startpos = tmp.find(L"=") + 1;
        font.KerningsList[k].firstid = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

        // get second character
        fs >> tmp; // second=45
        startpos = tmp.find(L"=") + 1;
        font.KerningsList[k].secondid = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

        // get amount
        fs >> tmp; // amount=-1
        startpos = tmp.find(L"=") + 1;
        int t = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos));
        font.KerningsList[k].amount = (float)t / (float)windowWidth;
    }

    return font;
}

void RenderText(DXGraphicsCommandList* commandList,
                ID3D12DescriptorHeap* dsDescriptorHeap,
                UINT frameIndex,
                Font font, 
                std::wstring text, 
                XMFLOAT2 pos, 
                XMFLOAT2 scale, 
                XMFLOAT2 padding, 
                XMFLOAT4 color)
{
    // clear the depth buffer so we can draw over everything
    commandList->ClearDepthStencilView(dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // set the text pipeline state object
    commandList->SetPipelineState(textPSO);

    // this way we only need 4 vertices per quad rather than 6 if we were to use a triangle list topology
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // set the text vertex buffer
    commandList->IASetVertexBuffers(0, 1, &textVertexBufferView[frameIndex]);

    // bind the text srv. We will assume the correct descriptor heap and table are currently bound and set
    commandList->SetGraphicsRootDescriptorTable(1, font.srvHandle);

    int numCharacters = 0;

    float topLeftScreenX = (pos.x * 2.0f) - 1.0f;
    float topLeftScreenY = ((1.0f - pos.y) * 2.0f) - 1.0f;

    float x = topLeftScreenX;
    float y = topLeftScreenY;

    float horrizontalPadding = (font.leftpadding + font.rightpadding) * padding.x;
    float verticalPadding = (font.toppadding + font.bottompadding) * padding.y;

    // cast the gpu virtual address to a textvertex, so we can directly store our vertices there
    TextVertex* vert = (TextVertex*)textVBGPUAddress[frameIndex];

    wchar_t lastChar = -1; // no last character to start with

    for (int i = 0; i < text.size(); ++i)
    {
        wchar_t c = text[i];

        FontChar* fc = font.GetChar(c);

        // character not in font char set
        if (fc == nullptr)
            continue;

        // end of string
        if (c == L' ')
            break;

        // new line
        if (c == L'n')
        {
            x = topLeftScreenX;
            y -= (font.lineHeight + verticalPadding) * scale.y;
            continue;
        }

        // don't overflow the buffer. In your app if this is true, you can implement a resize of your text vertex buffer
        if (numCharacters >= maxNumTextCharacters)
            break;

        float kerning = 0.0f;
        if (i > 0)
            kerning = font.GetKerning(lastChar, c);

        vert[numCharacters] = TextVertex(color.x,
            color.y,
            color.z,
            color.w,
            fc->u,
            fc->v,
            fc->twidth,
            fc->theight,
            x + ((fc->xoffset + kerning) * scale.x),
            y - (fc->yoffset * scale.y),
            fc->width * scale.x,
            fc->height * scale.y);

        numCharacters++;

        // remove horrizontal padding and advance to next char position
        x += (fc->xadvance - horrizontalPadding) * scale.x;

        lastChar = c;
    }

    // we are going to have 4 vertices per character (trianglestrip to make quad), and each instance is one character
    commandList->DrawInstanced(4, numCharacters, 0, 0);
}
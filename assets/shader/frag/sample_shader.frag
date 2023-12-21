#version 460

layout(binding = 3) uniform sampler2D texSampler;

layout(binding = 4) uniform Timer {
  float time;
} timer;

layout(binding = 5) uniform Mouse {
  vec2 pos;
} mouse;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPosition;

layout(location = 0) out vec4 outColor;

vec3 MixLayersByRed(in vec3 _layer1, in vec3 _layer2)
{
  float compR = step(_layer1.r, _layer2.r);

  return mix(_layer1, _layer2, compR);
}

vec3 MixLayersByBlue(in vec3 _layer1, in vec3 _layer2)
{
  float compB = step(_layer1.b, _layer2.b);

  return mix(_layer1, _layer2, compB);
}

vec3 MixLayersByBR(in vec3 _layer1, in vec3 _layer2)
{
  float compBR = step(_layer1.b + _layer1.r, _layer2.b + _layer2.r);

  return mix(_layer1, _layer2, compBR);
}

//パターンベクトルを返す
//args...
//  _gridSize: 格子の個数の平方根
vec2 Pattern(vec2 _st, in float _gridSize)
{
//  _st *= _gridSize;
  return fract(_st * _gridSize);
}

//所定位置を塗りつぶす
//args...
//  rgb: 塗りつぶす色
//  _st: グリッド空間の座標
//  _gridPos: 所定位置
vec3 Draw(in vec3 rgb, in vec2 _st, float _gridSize, in vec2 _gridPos)
{
  _st *= _gridSize;
  return rgb * step(_gridPos.x, _st.x) * step(_gridPos.y, _st.y)
            * (1. - step(_gridPos.x + 1., _st.x))
            * (1. - step(_gridPos.y + 1., _st.y));
}

vec3 MakeLayerType1(in vec3 _color, in vec2 _baseBox, in float _gridSize)
{
  vec3 layer = vec3(0.);

  float i;
  for (i = 0.; i < 3.; i += 1.)
    layer += Draw(_color, _baseBox, _gridSize, vec2(i));

  return layer;
}

vec3 MakeLayerType2(in vec3 _color, in vec2 _baseBox, in float _gridSize)
{
  vec3 layer = vec3(0.);

  float i;
  for (i = 0.; i < 2.; i += 1.)
  {
    layer += Draw(_color, _baseBox, _gridSize, vec2(i, 2.));
    layer += Draw(_color, _baseBox, _gridSize, vec2(i+1., 0.));
  }

  for (i = 0.; i < 3.; i += 2.)
    layer += Draw(_color, _baseBox, _gridSize, vec2(i, 1.));

  return layer;
}

vec3 MakeLayerType3(in vec3 _color, in vec2 _baseBox, in float _gridSize)
{
  vec3 layer = vec3(0.);

  float i;
  for (i = 0.; i < 2.; i += 1.)
  {
    layer += Draw(_color, _baseBox, _gridSize, vec2(i, i));
    layer += Draw(_color, _baseBox, _gridSize, vec2(i+1., i));
  }
  return layer;
}

vec3 MakeLayerType4(in vec3 _color, in vec2 _baseBox, in float _gridSize)
{
  vec3 layer = vec3(0.);

  float i;
  for (i = 0.; i < 2.; i += 1.)
    layer += Draw(_color, _baseBox, _gridSize, vec2(i));

  return layer;
}

vec3 MakeLayerType5(in vec3 _color, in vec2 _baseBox, in float _gridSize)
{
  vec3 layer = vec3(0.);

  float i;
  for (i = 0.; i < 2.; i += 1.)
    layer += Draw(_color, _baseBox, _gridSize, vec2(i, 2.));
  for (i = 1.; i < 3.; i += 1.)
    layer += Draw(_color, _baseBox, _gridSize, vec2(i, 1.));

  return layer;
}

vec3 MakeLayerType6(in vec3 _color, in vec2 _baseBox, in float _gridSize)
{
  vec3 layer = vec3(0.);

  float i;
  for (i = 1.; i < 3.; i += 1.)
    layer += Draw(_color, _baseBox, _gridSize, vec2(i));

  return layer;
}

const vec3 red = vec3(1., 0., 0.);
const vec3 blue = vec3(0., 0., 1.);
const vec3 white = vec3(1., 1., 1.);
const vec3 black = vec3(0., 0., 0.);

//キャンバスを作成する
//キャンバス: 仮想的な画面をそう呼ぶことにする
//レイヤもそう呼ぶが，キャンバスはレイヤを塗り分けたものと考える
vec3 CreateCanvas01(in vec3 _color, in vec2 _baseBox)
{
  //パターン
  vec2 baseBox = Pattern(_baseBox, 3.);

  //キャンバスを作成する
  //今回はレイヤをそのまま複製した
  vec3 canvas = MakeLayerType1(_color, baseBox, 3.);
  return canvas;
}

vec3 CreateCanvas02(in vec3 _color, in vec2 _baseBox)
{
  vec2 baseBox = Pattern(_baseBox, 3.);

  vec3 canvas = MakeLayerType2(_color, baseBox, 3.);
  return canvas;
}

vec3 CreateCanvas03(in vec3 _color, in vec2 _baseBox)
{
  vec3 canvas = vec3(0.);
  vec2 baseBox = Pattern(_baseBox, 3.);

  vec3 layer = MakeLayerType4(_color, baseBox, 3.);
  float i;
  for (i = 0.; i < 3.; i += 1.)
    canvas += Draw(layer, _baseBox, 3., vec2(0., i));

  return canvas;
}

vec3 CreateCanvas04(in vec3 _color, in vec2 _baseBox)
{
  vec3 canvas = vec3(0.);
  vec2 baseBox = Pattern(_baseBox, 3.);

  vec3 layer = MakeLayerType6(_color, baseBox, 3.);
  float i;
  for (i = 0.; i < 3.; i += 1.)
    canvas += Draw(layer, _baseBox, 3., vec2(2., i));

  return canvas;
}

vec3 CreateCanvas05(in vec3 _color, in vec2 _baseBox)
{
  vec3 canvas = vec3(0.);
  vec2 baseBox = Pattern(_baseBox, 3.);

  vec3 layer = MakeLayerType3(_color, baseBox, 3.);

  float i;
  for (i = 0.; i < 3.; i += 1.)
    canvas += Draw(layer, _baseBox, 3., vec2(i, 0.));

  return canvas;
}

vec3 CreateCanvas06(in vec3 _color, in vec2 _baseBox)
{
  vec3 canvas = vec3(0.);
  vec2 baseBox = Pattern(_baseBox, 3.);

  vec3 layer = MakeLayerType5(_color, baseBox, 3.);

  float i;
  for (i = 0.; i < 3.; i += 1.)
    canvas += Draw(layer, _baseBox, 3., vec2(i, 2.));

  return canvas;
}

vec3 CreateCanvas1(in vec2 _baseBox)
{
  vec3 canvas = vec3(0.);
  vec2 baseBox = Pattern(_baseBox, 3.);

  vec3 layer1 = CreateCanvas01(red, baseBox);
  vec3 layer2 = CreateCanvas02(red, baseBox);
  vec3 layer3 = MixLayersByBlue(CreateCanvas03(blue, baseBox), layer1);
  vec3 layer4 = MixLayersByBlue(CreateCanvas05(blue, baseBox), layer2);

  float i;
  for (i = 0.; i < 3.; i += 1.)
  {
    canvas += Draw(layer1, _baseBox, 3., vec2(1., i));
    canvas = MixLayersByBR(canvas, Draw(layer3, _baseBox, 3., vec2(0., i)));
    canvas = MixLayersByBR(canvas, Draw(layer4, _baseBox, 3., vec2(i, 0.)));
    canvas += Draw(layer2, _baseBox, 3., vec2(i, 1.));
  }

  return canvas;
}

vec3 CreateCanvas2(in vec2 _baseBox)
{
  vec3 canvas = vec3(0.);
  vec2 baseBox = Pattern(_baseBox, 3.);

  vec3 layer1 = CreateCanvas01(red, baseBox);
  vec3 layer2 = CreateCanvas02(white, baseBox);
  vec3 layer3 = MixLayersByBlue(CreateCanvas03(blue, baseBox), layer1);

  float i;
  vec3 temp = vec3(0.);
  for (i = 0.; i < 3.; i += 1.)
  {
    canvas += Draw(layer3, _baseBox, 3., vec2(0., i));
    canvas += Draw(layer1, _baseBox, 3., vec2(1., i));
    temp += Draw(layer2, _baseBox, 3., vec2(i, 0.));
    temp += Draw(layer2, _baseBox, 3., vec2(i, 2.));
    canvas = MixLayersByBR(canvas, temp);
  }

  return canvas;
}

vec3 CreateCanvas3(in vec2 _baseBox)
{
  vec3 canvas = vec3(0.);
  vec2 baseBox = Pattern(_baseBox, 3.);

  vec3 layer1 = CreateCanvas01(red, baseBox);
  vec3 layer2 = CreateCanvas02(red, baseBox);
  vec3 layer3 = MixLayersByBlue(CreateCanvas03(blue, baseBox), layer1);
  vec3 layer4 = MixLayersByBlue(CreateCanvas06(blue, baseBox), layer2);

  float i;
  for (i = 0.; i < 3.; i += 1.)
  {
    canvas += Draw(layer2, _baseBox, 3., vec2(i, 1.));
    canvas = MixLayersByBR(Draw(layer1, _baseBox, 3., vec2(1., i)), canvas);
    canvas = MixLayersByBR(Draw(layer4, _baseBox, 3., vec2(i, 2.)), canvas);
   canvas = MixLayersByBR(Draw(layer3, _baseBox, 3., vec2(0., i)), canvas);
  }

  return canvas;
}

vec3 CreateCanvas4(in vec2 _baseBox)
{
  vec3 canvas = vec3(0.);
  vec2 baseBox = Pattern(_baseBox, 3.);

  vec3 layer1 = CreateCanvas01(white, baseBox);
  vec3 layer2 = CreateCanvas02(red, baseBox);
  vec3 layer3 = MixLayersByBlue(CreateCanvas05(blue, baseBox), layer2);

  float i;
  for (i = 0.; i < 3.; i += 1.)
  {
    canvas += Draw(layer1, _baseBox, 3., vec2(0., i));
    canvas += Draw(layer1, _baseBox, 3., vec2(2., i));
    canvas += Draw(layer2, _baseBox, 3., vec2(i, 1.));
    canvas = MixLayersByBR(canvas, Draw(layer3, _baseBox, 3., vec2(i, 0.)));
  }

  return canvas;
}

vec3 CreateCanvas5(in vec2 _baseBox)
{
  vec3 canvas = vec3(0.);
  vec2 baseBox = Pattern(_baseBox, 3.);

  vec3 layer1 = CreateCanvas01(white, baseBox);
  vec3 layer2 = CreateCanvas02(white, baseBox);

  float i;
  for (i = 0.; i < 3.; i += 1.)
  {
    canvas += Draw(layer1, _baseBox, 3., vec2(0., i));
    canvas += Draw(layer1, _baseBox, 3., vec2(2., i));
    canvas += Draw(layer2, _baseBox, 3., vec2(i, 0.));
    canvas += Draw(layer2, _baseBox, 3., vec2(i, 2.));
  }

  return canvas;
}

vec3 CreateCanvas6(in vec2 _baseBox)
{
  vec3 canvas = vec3(0.);
  vec2 baseBox = Pattern(_baseBox, 3.);

  vec3 layer1 = CreateCanvas01(white, baseBox);
  vec3 layer2 = CreateCanvas02(red, baseBox);
  vec3 layer3 = MixLayersByBlue(CreateCanvas06(blue, baseBox), layer2);

  float i;
  for (i = 0.; i < 3.; i += 1.)
  {
    canvas += Draw(layer1, _baseBox, 3., vec2(0., i));
    canvas += Draw(layer1, _baseBox, 3., vec2(2., i));
    canvas += Draw(layer2, _baseBox, 3., vec2(i, 1.));
    canvas = MixLayersByBR(canvas, Draw(layer3, _baseBox, 3., vec2(i, 2.)));
  }

  return canvas;
}

vec3 CreateCanvas7(in vec2 _baseBox)
{
  vec3 canvas = vec3(0.);
  vec2 baseBox = Pattern(_baseBox, 3.);

  vec3 layer1 = CreateCanvas01(red, baseBox);
  vec3 layer2 = CreateCanvas02(red, baseBox);
  vec3 layer3 = MixLayersByBlue(CreateCanvas04(blue, baseBox), layer1);
  vec3 layer4 = MixLayersByBlue(CreateCanvas05(blue, baseBox), layer2);

  float i;
  for (i = 0.; i < 3.; i += 1.)
  {
    canvas += Draw(layer1, _baseBox, 3., vec2(1., i));
    canvas = MixLayersByBR(canvas, Draw(layer3, _baseBox, 3., vec2(2., i)));
    canvas = MixLayersByBR(canvas, Draw(layer4, _baseBox, 3., vec2(i, 0.)));
    canvas += Draw(layer2, _baseBox, 3., vec2(i, 1.));
  }

  return canvas;
}

vec3 CreateCanvas8(in vec2 _baseBox)
{
  vec3 canvas = vec3(0.);
  vec2 baseBox = Pattern(_baseBox, 3.);

  vec3 layer1 = CreateCanvas01(red, baseBox);
  vec3 layer2 = CreateCanvas02(white, baseBox);
  vec3 layer3 = MixLayersByBlue(CreateCanvas04(blue, baseBox), layer1);

  float i;
  for (i = 0.; i < 3.; i += 1.)
  {
    canvas += Draw(layer1, _baseBox, 3., vec2(1., i));
    canvas = MixLayersByBR(canvas, Draw(layer3, _baseBox, 3., vec2(2., i)));
    canvas += Draw(layer2, _baseBox, 3., vec2(i, 0.));
    canvas += Draw(layer2, _baseBox, 3., vec2(i, 2.));
  }

  return canvas;
}

vec3 CreateCanvas9(in vec2 _baseBox)
{
  vec3 canvas = vec3(0.);
  vec2 baseBox = Pattern(_baseBox, 3.);

  vec3 layer1 = CreateCanvas01(red, baseBox);
  vec3 layer2 = CreateCanvas02(red, baseBox);
  vec3 layer3 = MixLayersByBlue(CreateCanvas04(blue, baseBox), layer1);
  vec3 layer4 = MixLayersByBlue(CreateCanvas06(blue, baseBox), layer2);

  float i;
  for (i = 0.; i < 3.; i += 1.)
  {
    canvas += Draw(layer2, _baseBox, 3., vec2(i, 1.));
    canvas = MixLayersByBR(canvas, Draw(layer3, _baseBox, 3., vec2(2., i)));
    canvas = MixLayersByBR(canvas, Draw(layer1, _baseBox, 3., vec2(1, i)));
    canvas = MixLayersByBR(canvas, Draw(layer4, _baseBox, 3., vec2(i, 2.)));
  }

  return canvas;
}

vec3 CreateCanvas(in vec2 _baseBox)
{
  vec3 canvas = vec3(0.);
  vec2 baseBox = Pattern(_baseBox, 3.);

  canvas += Draw(CreateCanvas1(baseBox), _baseBox, 3., vec2(0.));
  canvas += Draw(CreateCanvas2(baseBox), _baseBox, 3., vec2(0., 1.));
  canvas += Draw(CreateCanvas3(baseBox), _baseBox, 3., vec2(0., 2.));
  canvas += Draw(CreateCanvas4(baseBox), _baseBox, 3., vec2(1., 0.));
  canvas += Draw(CreateCanvas5(baseBox), _baseBox, 3., vec2(1.));
  canvas += Draw(CreateCanvas6(baseBox), _baseBox, 3., vec2(1., 2.));
  canvas += Draw(CreateCanvas7(baseBox), _baseBox, 3., vec2(2., 0.));
  canvas += Draw(CreateCanvas8(baseBox), _baseBox, 3., vec2(2., 1.));
  canvas += Draw(CreateCanvas9(baseBox), _baseBox, 3., vec2(2.));
  return canvas;
}

void main()
{
  vec2 st = fragPosition.xy;
  vec3 color = vec3(0., 0., 0.);

  vec2 _st = Pattern(st, round(length(mouse.pos + 0.2f) / 100.0));
  color = CreateCanvas(st);
  color = CreateCanvas(_st);

  outColor = sin(timer.time * 0.7f) * vec4(color, 1.)
          + cos(timer.time * 0.8f) * texture(texSampler, fragTexCoord);
}
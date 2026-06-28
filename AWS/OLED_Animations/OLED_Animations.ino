#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// Define custom I2C pins for ESP8266 OLED
#ifndef D5
#define D5 14 // GPIO14 (SDA)
#endif
#ifndef D6
#define D6 12 // GPIO12 (SCL)
#endif

#define SDA_PIN D5
#define SCL_PIN D6

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Animation state variables
unsigned long lastFrameTime = 0;
const int frameDelay = 30; // ~33 FPS
int currentAnimation = 0;
unsigned long animationStartTime = 0;
const unsigned long animationDuration = 10000; // Switch every 10 seconds

// ----------------------------------------------------
// Animation 1: Starfield (3D space simulation)
// ----------------------------------------------------
const int numStars = 40;
struct Star {
  float x, y, z;
  float prevX, prevY, prevZ;
};
Star stars[numStars];

void initStarfield() {
  for (int i = 0; i < numStars; i++) {
    stars[i].x = random(-100, 100);
    stars[i].y = random(-50, 50);
    stars[i].z = random(10, 150);
    stars[i].prevZ = stars[i].z;
  }
}

void drawStarfield() {
  display.clearDisplay();
  
  float halfWidth = SCREEN_WIDTH / 2.0;
  float halfHeight = SCREEN_HEIGHT / 2.0;
  float speed = 2.0;

  for (int i = 0; i < numStars; i++) {
    stars[i].prevZ = stars[i].z;
    stars[i].z -= speed;

    if (stars[i].z <= 1) {
      stars[i].x = random(-100, 100);
      stars[i].y = random(-50, 50);
      stars[i].z = 150;
      stars[i].prevZ = stars[i].z;
    }

    // 3D Projection
    float px = (stars[i].x * 64.0) / stars[i].z + halfWidth;
    float py = (stars[i].y * 64.0) / stars[i].z + halfHeight;

    float prevPx = (stars[i].x * 64.0) / stars[i].prevZ + halfWidth;
    float prevPy = (stars[i].y * 64.0) / stars[i].prevZ + halfHeight;

    // Out of bounds check
    if (px < 0 || px >= SCREEN_WIDTH || py < 0 || py >= SCREEN_HEIGHT) {
      stars[i].x = random(-100, 100);
      stars[i].y = random(-50, 50);
      stars[i].z = 150;
      continue;
    }

    // Draw star trails (lines) and the star itself (dot)
    display.drawLine((int)prevPx, (int)prevPy, (int)px, (int)py, SSD1306_WHITE);
    display.drawPixel((int)px, (int)py, SSD1306_WHITE);
  }
}

// ----------------------------------------------------
// Animation 2: Rotating 3D Wireframe Cube
// ----------------------------------------------------
struct Point3D {
  float x, y, z;
};

const Point3D cubeVertices[8] = {
  {-16, -16, -16}, {16, -16, -16}, {16, 16, -16}, {-16, 16, -16},
  {-16, -16, 16},  {16, -16, 16},  {16, 16, 16},  {-16, 16, 16}
};

const int cubeEdges[12][2] = {
  {0, 1}, {1, 2}, {2, 3}, {3, 0}, // Back face
  {4, 5}, {5, 6}, {6, 7}, {7, 4}, // Front face
  {0, 4}, {1, 5}, {2, 6}, {3, 7}  // Connecting edges
};

float angleX = 0;
float angleY = 0;
float angleZ = 0;

void drawCube() {
  display.clearDisplay();
  
  Point3D projected[8];
  float halfWidth = SCREEN_WIDTH / 2.0;
  float halfHeight = SCREEN_HEIGHT / 2.0;

  // Compute rotation matrix elements
  float radX = angleX * PI / 180.0;
  float radY = angleY * PI / 180.0;
  float radZ = angleZ * PI / 180.0;

  float cosX = cos(radX), sinX = sin(radX);
  float cosY = cos(radY), sinY = sin(radY);
  float cosZ = cos(radZ), sinZ = sin(radZ);

  for (int i = 0; i < 8; i++) {
    // Rotate on X axis
    float y1 = cubeVertices[i].y * cosX - cubeVertices[i].z * sinX;
    float z1 = cubeVertices[i].y * sinX + cubeVertices[i].z * cosX;

    // Rotate on Y axis
    float x2 = cubeVertices[i].x * cosY + z1 * sinY;
    float z2 = -cubeVertices[i].x * sinY + z1 * cosY;

    // Rotate on Z axis
    float x3 = x2 * cosZ - y1 * sinZ;
    float y3 = x2 * sinZ + y1 * cosZ;

    // Perspective projection (camera distance = 60)
    float distance = 60.0;
    float scale = distance / (distance + z2);
    projected[i].x = x3 * scale + halfWidth;
    projected[i].y = y3 * scale + halfHeight;
  }

  // Draw Edges (Lines)
  for (int i = 0; i < 12; i++) {
    int startNode = cubeEdges[i][0];
    int endNode = cubeEdges[i][1];
    display.drawLine(
      (int)projected[startNode].x, (int)projected[startNode].y,
      (int)projected[endNode].x, (int)projected[endNode].y,
      SSD1306_WHITE
    );
  }

  // Draw Vertices (Dots)
  for (int i = 0; i < 8; i++) {
    display.fillCircle((int)projected[i].x, (int)projected[i].y, 2, SSD1306_WHITE);
  }

  // Increment rotation angles
  angleX += 1.5;
  angleY += 2.0;
  angleZ += 1.0;

  if (angleX >= 360) angleX -= 360;
  if (angleY >= 360) angleY -= 360;
  if (angleZ >= 360) angleZ -= 360;
}

// ----------------------------------------------------
// Animation 3: Morphing Wave Ribbon & Particles
// ----------------------------------------------------
float waveOffset = 0;

void drawWaves() {
  display.clearDisplay();

  // Draw morphing wave lines
  for (int w = 0; w < 3; w++) {
    float shift = w * 0.4;
    int prevY = -1;

    for (int x = 0; x < SCREEN_WIDTH; x += 2) {
      float angle = (x * 0.05) + waveOffset + shift;
      int y = SCREEN_HEIGHT / 2 + (int)(15.0 * sin(angle) * cos(angle * 0.3));

      if (x > 0 && prevY != -1) {
        display.drawLine(x - 2, prevY, x, y, SSD1306_WHITE);
      }
      prevY = y;
    }
  }

  // Draw floating particles (dots) orbiting the wave
  for (int i = 0; i < 5; i++) {
    float pAngle = waveOffset + (i * (2.0 * PI / 5.0));
    int px = SCREEN_WIDTH / 2 + (int)(45.0 * cos(pAngle));
    int py = SCREEN_HEIGHT / 2 + (int)(12.0 * sin(pAngle * 2.0));
    display.fillCircle(px, py, 2, SSD1306_WHITE);
    // Draw trail line for each dot
    display.drawLine(px, py, px - (int)(5 * cos(pAngle)), py - (int)(3 * sin(pAngle)), SSD1306_WHITE);
  }

  waveOffset += 0.04;
}

// ----------------------------------------------------
// Animation 4: Mathematical Spirograph (Lissajous)
// ----------------------------------------------------
float spiroTime = 0;
const int maxSpiroPoints = 50;
int spiroX[maxSpiroPoints];
int spiroY[maxSpiroPoints];
int spiroIndex = 0;
bool spiroBufferFull = false;

void drawSpirograph() {
  display.clearDisplay();

  // Calculate new point
  // Harmonized Lissajous / Spirograph coordinates
  float r1 = 25.0;
  float r2 = 12.0;
  float f1 = 2.0;
  float f2 = 5.0;
  float f3 = 3.0;

  float cx = SCREEN_WIDTH / 2.0;
  float cy = SCREEN_HEIGHT / 2.0;

  float x = cx + r1 * cos(f1 * spiroTime) + r2 * sin(f2 * spiroTime);
  float y = cy + r1 * sin(f1 * spiroTime) + r2 * cos(f3 * spiroTime);

  // Store in circular buffer
  spiroX[spiroIndex] = (int)x;
  spiroY[spiroIndex] = (int)y;

  spiroIndex = (spiroIndex + 1) % maxSpiroPoints;
  if (spiroIndex == 0) spiroBufferFull = true;

  int count = spiroBufferFull ? maxSpiroPoints : spiroIndex;

  // Draw lines connecting the trace points
  for (int i = 0; i < count - 1; i++) {
    int idx1 = (spiroIndex - count + i + maxSpiroPoints) % maxSpiroPoints;
    int idx2 = (idx1 + 1) % maxSpiroPoints;
    display.drawLine(spiroX[idx1], spiroY[idx1], spiroX[idx2], spiroY[idx2], SSD1306_WHITE);
  }

  // Draw dots at the current head and some key joints
  int headIdx = (spiroIndex - 1 + maxSpiroPoints) % maxSpiroPoints;
  display.fillCircle(spiroX[headIdx], spiroY[headIdx], 3, SSD1306_WHITE);
  
  if (count > 10) {
    int midIdx = (spiroIndex - 10 + maxSpiroPoints) % maxSpiroPoints;
    display.drawCircle(spiroX[midIdx], spiroY[midIdx], 2, SSD1306_WHITE);
  }

  spiroTime += 0.03;
}

// ----------------------------------------------------
// Animation 5: Particle Rain & Splash
// ----------------------------------------------------
const int numRain = 12;
struct RainDrop {
  float x;
  float y;
  float speed;
  float length;
  int splashRadius;
  int splashMaxRadius;
};
RainDrop rain[numRain];

void initRain() {
  for (int i = 0; i < numRain; i++) {
    rain[i].x = random(0, SCREEN_WIDTH);
    rain[i].y = random(-50, 0);
    rain[i].speed = random(3, 6);
    rain[i].length = random(4, 10);
    rain[i].splashRadius = 0;
    rain[i].splashMaxRadius = random(4, 8);
  }
}

void drawRain() {
  display.clearDisplay();

  for (int i = 0; i < numRain; i++) {
    if (rain[i].splashRadius == 0) {
      // Fall down
      rain[i].y += rain[i].speed;

      // Draw drop (line) and leading edge (dot)
      display.drawLine((int)rain[i].x, (int)(rain[i].y - rain[i].length), (int)rain[i].x, (int)rain[i].y, SSD1306_WHITE);
      display.drawPixel((int)rain[i].x, (int)rain[i].y, SSD1306_WHITE);

      // Hit bottom splash trigger
      if (rain[i].y >= SCREEN_HEIGHT - 4) {
        rain[i].y = SCREEN_HEIGHT - 1;
        rain[i].splashRadius = 1;
      }
    } else {
      // Splash mode (expanding dots/rings)
      display.drawCircle((int)rain[i].x, (int)rain[i].y, rain[i].splashRadius, SSD1306_WHITE);
      
      // Draw splash particles (4 diagonal dots)
      display.drawPixel((int)rain[i].x - rain[i].splashRadius, (int)rain[i].y - rain[i].splashRadius/2, SSD1306_WHITE);
      display.drawPixel((int)rain[i].x + rain[i].splashRadius, (int)rain[i].y - rain[i].splashRadius/2, SSD1306_WHITE);

      rain[i].splashRadius += 1;

      if (rain[i].splashRadius > rain[i].splashMaxRadius) {
        // Reset drop
        rain[i].x = random(0, SCREEN_WIDTH);
        rain[i].y = random(-50, -10);
        rain[i].speed = random(3, 6);
        rain[i].length = random(4, 10);
        rain[i].splashRadius = 0;
        rain[i].splashMaxRadius = random(4, 8);
      }
    }
  }
}

// ----------------------------------------------------
// Setup and Loop
// ----------------------------------------------------
void setup() {
  Serial.begin(115200);
  
  // Initialize I2C with custom SDA and SCL pins
  Wire.begin(SDA_PIN, SCL_PIN);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.clearDisplay();
  display.display();

  // Initialize random seed and animations
  randomSeed(analogRead(0));
  initStarfield();
  initRain();

  animationStartTime = millis();
}

void loop() {
  unsigned long currentTime = millis();

  // Run the current animation frame
  if (currentTime - lastFrameTime >= frameDelay) {
    lastFrameTime = currentTime;

    switch (currentAnimation) {
      case 0:
        drawStarfield();
        break;
      case 1:
        drawCube();
        break;
      case 2:
        drawWaves();
        break;
      case 3:
        drawSpirograph();
        break;
      case 4:
        drawRain();
        break;
    }
    
    display.display();
  }

  // Animation cycle timer
  if (currentTime - animationStartTime >= animationDuration) {
    animationStartTime = currentTime;
    
    // Switch to next animation
    currentAnimation = (currentAnimation + 1) % 5;
    
    // Re-initialize states for specific animations if needed
    if (currentAnimation == 0) {
      initStarfield();
    } else if (currentAnimation == 3) {
      spiroIndex = 0;
      spiroBufferFull = false;
      spiroTime = 0;
    } else if (currentAnimation == 4) {
      initRain();
    }
  }
}

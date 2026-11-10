#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

static void die(const char *s) {
  fprintf(stderr, "%s\n", s);
  exit(1);
}

int N = 801;
int **grid;

// you can also use this function to debug print your sandpile
static void render(char *filename) {
  int chan = 3;
  unsigned char *pixels = (unsigned char *)malloc(N * N * chan * sizeof(char));
  int stride = N * chan;
  if (!pixels)
    die("malloc failed for PNG pixel buffer");
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
      unsigned char r = 0, g = 0, b = 0;
      switch (grid[i][j]) {
      case 0: // obsidian
        r = 15;
        g = 15;
        b = 25;
        break;
      case 1: // electric cyan
        r = 0;
        g = 210;
        b = 255;
        break;
      case 2: // vibrant magenta
        r = 230;
        g = 30;
        b = 130;
        break;
      case 3: // bright gold
        r = 255;
        g = 200;
        b = 0;
        break;
      default: // white
        r = 0;
        g = 0;
        b = 0;
      }

      int pid = (i * N + j) * chan;
      pixels[pid] = r;
      pixels[pid + 1] = g;
      pixels[pid + 2] = b;
    }
  }
  if (stbi_write_png(filename, N, N, chan, pixels, stride)) {
    printf("Successfully saved sandpile identity to: %s\n", filename);
  } else {
    die("Error: stb_image_write failed to save png file\n");
  }
  free(pixels);
}

static void stabilize() {
  const int n = N;
  const int c = n / 2;
  const int r2 = c * c;
  const int w = n + 2;                     // padded width: 1-cell border all around
  const size_t sz = (size_t)w * w;

  int *restrict a = calloc(sz, sizeof(int));    // current grid
  int *restrict b = calloc(sz, sizeof(int));    // next grid
  int *restrict m = calloc(sz, sizeof(int));    // 1 = inside Pacman, 0 = outside or border

  for (int i = 0; i < n; ++i) {
    int jmax = MAX(i, n - 1 - i);
    for (int j = 0; j < n; ++j) {
      size_t k = (size_t)(i + 1) * w + (j + 1);
      a[k] = grid[i][j];
      m[k] = (j <= jmax) &&
             ((i - c) * (i - c) + (j - c) * (j - c) <= r2);
    }
  }

  int changed;
  do {
    changed = 0;
    for (int i = 1; i <= n; ++i) {
      for (int j = 1; j <= n; ++j) {
        size_t k = (size_t)i * w + j;
        int q  = (a[k]     >> 2) & -m[k];
        int qu = (a[k - w] >> 2) & -m[k - w];
        int qd = (a[k + w] >> 2) & -m[k + w];
        int ql = (a[k - 1] >> 2) & -m[k - 1];
        int qr = (a[k + 1] >> 2) & -m[k + 1];
        b[k] = a[k] - 4 * q + qu + qd + ql + qr;
        changed |= q;
      }
    }
    int *t = a; a = b; b = t;   // swap buffers
  } while (changed);

  // Copy result back.
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j)
      grid[i][j] = a[(size_t)(i + 1) * w + (j + 1)];

  free(a);
  free(b);
  free(m);
}

void find_identity() {
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
      grid[i][j] = 6;
    }
  }
  stabilize();
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
      grid[i][j] = 6 - grid[i][j];
    }
  }
  stabilize();
}

int main(int argc, char **argv) {
  if (argc < 2 || argc > 2) {
    fprintf(stderr, "Usage: %s N\n", argv[0]);
    return EXIT_FAILURE;
  }
  char *end;
  uintmax_t parsed = strtoumax(argv[1], &end, 10);
  if (errno || argv[1][0] < '0' || argv[1][0] > '9' || *end || !parsed ||
      parsed > INT32_MAX)
    die("grid size must be a positive int");
  N = (int)parsed;

  grid = (int **)malloc(N * sizeof(int *));
  for (int i = 0; i < N; ++i) {
    grid[i] = (int *)malloc(N * sizeof(int));
  }

  find_identity();

  render("identity.png");
  for (int i = 0; i < N; ++i) {
    free(grid[i]);
  }
  free(grid);
  return EXIT_SUCCESS;
}

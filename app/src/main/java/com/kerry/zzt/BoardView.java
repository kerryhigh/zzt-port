package com.kerry.zzt;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.view.View;

import java.util.ArrayDeque;
import java.util.Arrays;

final class BoardView extends View {
    interface CellTapListener {
        void onCellTapped(int x, int y);
    }

    static final int COLS = BoardRasterizer.COLS;
    static final int ROWS = BoardRasterizer.ROWS;
    static final int CELL_W = BoardRasterizer.CELL_W;
    static final int CELL_H = BoardRasterizer.CELL_H;
    private static final int WIDTH = BoardRasterizer.WIDTH;
    private static final int HEIGHT = BoardRasterizer.HEIGHT;
    private static final int CELL_COUNT = COLS * ROWS;
    private static final long TRANSITION_PHASE_MS = 64;
    private static final int TRANSITION_BOARD_CHANGE = 1;
    private static final int TRANSITION_FILL = 2;
    private static final int TRANSITION_REDRAW = 3;
    private final Bitmap bitmap = Bitmap.createBitmap(WIDTH, HEIGHT, Bitmap.Config.ARGB_8888);
    private final Bitmap transitionBitmap = Bitmap.createBitmap(WIDTH, HEIGHT, Bitmap.Config.ARGB_8888);
    private final Paint paint = new Paint();
    private final Paint cursorPaint = new Paint();
    private final int[] pixels = new int[WIDTH * HEIGHT];
    private final byte[] cells = new byte[COLS * ROWS * 2];
    private final byte[] transitionCells = new byte[COLS * ROWS * 2];
    private final byte[] font;
    private final ArrayDeque<BoardTransition> transitionQueue = new ArrayDeque<>();
    private long blinkAt;
    private long transitionStartedAt;
    private BoardTransition activeTransition;
    private CellTapListener cellTapListener;
    private int editorCursorX = -1;
    private int editorCursorY = -1;
    private boolean continuousCellMode;
    private boolean monochrome;
    private int lastTouchCellX = -1;
    private int lastTouchCellY = -1;

    BoardView(Context context, byte[] font) {
        super(context);
        this.font = Arrays.copyOf(font, font.length);
        paint.setFilterBitmap(false);
        paint.setAntiAlias(false);
        cursorPaint.setColor(0xff55ffff);
        cursorPaint.setStyle(Paint.Style.STROKE);
        cursorPaint.setAntiAlias(false);
        setLayerType(View.LAYER_TYPE_SOFTWARE, null);
        setFocusable(true);
        setFocusableInTouchMode(true);
    }

    void update(byte[] frame, long nowMillis) {
        if (frame.length < cells.length) return;
        System.arraycopy(frame, 0, cells, 0, cells.length);
        blinkAt = nowMillis;
        renderCells(cells, bitmap);
        advanceTransition(nowMillis);
        if (activeTransition != null) renderTransition(nowMillis);
        invalidate();
    }

    void enqueueTransition(byte[] from, byte[] to, short[] order, int type, long nowMillis) {
        if (from.length < cells.length || to.length < cells.length || order.length < CELL_COUNT) return;
        if (type < TRANSITION_BOARD_CHANGE || type > TRANSITION_REDRAW) return;
        BoardTransition transition = new BoardTransition(from, to, order, type);
        if (activeTransition == null) {
            activeTransition = transition;
            transitionStartedAt = nowMillis;
        } else {
            transitionQueue.addLast(transition);
        }
        invalidate();
    }

    void setCellTapListener(CellTapListener listener) {
        cellTapListener = listener;
        setClickable(listener != null);
        if (listener == null) setEditorCursor(-1, -1);
    }

    void setEditorCursor(int x, int y) {
        if (editorCursorX == x && editorCursorY == y) return;
        editorCursorX = x;
        editorCursorY = y;
        invalidate();
    }

    void setContinuousCellMode(boolean enabled) {
        continuousCellMode = enabled;
        lastTouchCellX = -1;
        lastTouchCellY = -1;
    }

    boolean isMonochrome() {
        return monochrome;
    }

    void setMonochrome(boolean enabled) {
        if (monochrome == enabled) return;
        monochrome = enabled;
        activeTransition = null;
        transitionQueue.clear();
        renderCells(cells, bitmap);
        invalidate();
    }

    private void renderCells(byte[] source, Bitmap target) {
        BoardRasterizer.rasterize(source, font, pixels, monochrome, blinkAt);
        target.setPixels(pixels, 0, WIDTH, 0, 0, WIDTH, HEIGHT);
    }

    private void advanceTransition(long nowMillis) {
        while (activeTransition != null
                && nowMillis - transitionStartedAt >= transitionDuration(activeTransition)) {
            transitionStartedAt += transitionDuration(activeTransition);
            activeTransition = transitionQueue.pollFirst();
        }
    }

    private void renderTransition(long nowMillis) {
        long elapsed = Math.max(0, nowMillis - transitionStartedAt);
        BoardTransition transition = activeTransition;
        if (transition.type == TRANSITION_BOARD_CHANGE && elapsed < TRANSITION_PHASE_MS) {
            System.arraycopy(transition.from, 0, transitionCells, 0, cells.length);
            int filled = (int) (elapsed * CELL_COUNT / TRANSITION_PHASE_MS);
            for (int i = 0; i < filled; i++) {
                int offset = (transition.order[i] & 0xffff) * 2;
                transitionCells[offset] = (byte) 0xdb;
                transitionCells[offset + 1] = 0x05;
            }
        } else if (transition.type == TRANSITION_BOARD_CHANGE) {
            for (int i = 0; i < CELL_COUNT; i++) {
                int offset = i * 2;
                transitionCells[offset] = (byte) 0xdb;
                transitionCells[offset + 1] = 0x05;
            }
            for (int x = 0; x < COLS; x++) {
                copyTransitionCell(transition, x, 0);
                copyTransitionCell(transition, x, ROWS - 1);
            }
            for (int y = 1; y < ROWS - 1; y++) {
                copyTransitionCell(transition, 0, y);
                copyTransitionCell(transition, COLS - 1, y);
            }
            int revealed = (int) ((elapsed - TRANSITION_PHASE_MS) * CELL_COUNT / TRANSITION_PHASE_MS);
            revealed = Math.min(CELL_COUNT, revealed);
            for (int i = 0; i < revealed; i++) {
                int offset = (transition.order[i] & 0xffff) * 2;
                transitionCells[offset] = transition.to[offset];
                transitionCells[offset + 1] = transition.to[offset + 1];
            }
        } else {
            System.arraycopy(transition.from, 0, transitionCells, 0, cells.length);
            if (transition.type == TRANSITION_REDRAW) {
                for (int x = 0; x < COLS; x++) {
                    copyTransitionCell(transition, x, 0);
                    copyTransitionCell(transition, x, ROWS - 1);
                }
                for (int y = 1; y < ROWS - 1; y++) {
                    copyTransitionCell(transition, 0, y);
                    copyTransitionCell(transition, COLS - 1, y);
                }
            }
            int updated = (int) (elapsed * CELL_COUNT / TRANSITION_PHASE_MS);
            updated = Math.min(CELL_COUNT, updated);
            for (int i = 0; i < updated; i++) {
                int offset = (transition.order[i] & 0xffff) * 2;
                transitionCells[offset] = transition.to[offset];
                transitionCells[offset + 1] = transition.to[offset + 1];
            }
        }
        renderCells(transitionCells, transitionBitmap);
    }

    private long transitionDuration(BoardTransition transition) {
        return transition.type == TRANSITION_BOARD_CHANGE
                ? TRANSITION_PHASE_MS * 2 : TRANSITION_PHASE_MS;
    }

    private void copyTransitionCell(BoardTransition transition, int x, int y) {
        int offset = (y * COLS + x) * 2;
        transitionCells[offset] = transition.to[offset];
        transitionCells[offset + 1] = transition.to[offset + 1];
    }

    @Override protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int availableWidth = MeasureSpec.getSize(widthMeasureSpec);
        int width = Math.min(availableWidth, WIDTH * 3);
        int height = width * HEIGHT / WIDTH;
        setMeasuredDimension(width, height);
    }

    @Override protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        canvas.drawColor(0xff000000);
        float scale = Math.min((float) getWidth() / WIDTH, (float) getHeight() / HEIGHT);
        float drawWidth = WIDTH * scale;
        float drawHeight = HEIGHT * scale;
        float left = (getWidth() - drawWidth) / 2f;
        float top = (getHeight() - drawHeight) / 2f;
        Bitmap displayBitmap = activeTransition == null ? bitmap : transitionBitmap;
        canvas.drawBitmap(displayBitmap, null,
                new android.graphics.RectF(left, top, left + drawWidth, top + drawHeight), paint);
        if (editorCursorX >= 1 && editorCursorX <= COLS && editorCursorY >= 1 && editorCursorY <= ROWS) {
            float cellLeft = left + (editorCursorX - 1) * CELL_W * scale;
            float cellTop = top + (editorCursorY - 1) * CELL_H * scale;
            float inset = Math.max(0.5f, scale * 0.6f);
            cursorPaint.setStrokeWidth(Math.max(1f, scale * 1.1f));
            canvas.drawRect(cellLeft + inset, cellTop + inset,
                    cellLeft + CELL_W * scale - inset, cellTop + CELL_H * scale - inset, cursorPaint);
        }
    }

    @Override public boolean onTouchEvent(android.view.MotionEvent event) {
        if (cellTapListener == null) return super.onTouchEvent(event);
        int action = event.getActionMasked();
        if (action == android.view.MotionEvent.ACTION_DOWN) {
            requestFocus();
            lastTouchCellX = -1;
            lastTouchCellY = -1;
            if (continuousCellMode) dispatchTouchCell(event.getX(), event.getY());
            return true;
        }
        if (action == android.view.MotionEvent.ACTION_MOVE) {
            if (continuousCellMode) dispatchTouchCell(event.getX(), event.getY());
            return true;
        }
        if (action == android.view.MotionEvent.ACTION_UP) {
            int cellX = touchCellX(event.getX());
            int cellY = touchCellY(event.getY());
            if (!continuousCellMode || cellX != lastTouchCellX || cellY != lastTouchCellY) {
                dispatchTouchCell(event.getX(), event.getY());
            }
            lastTouchCellX = -1;
            lastTouchCellY = -1;
            return true;
        }
        return true;
    }

    private void dispatchTouchCell(float touchX, float touchY) {
        int cellX = touchCellX(touchX);
        int cellY = touchCellY(touchY);
        if (cellX < 1 || cellX > COLS || cellY < 1 || cellY > ROWS) {
            lastTouchCellX = -1;
            lastTouchCellY = -1;
            return;
        }
        if (cellX == lastTouchCellX && cellY == lastTouchCellY) return;
        lastTouchCellX = cellX;
        lastTouchCellY = cellY;
        cellTapListener.onCellTapped(cellX, cellY);
    }

    private int touchCellX(float touchX) {
        float scale = Math.min((float) getWidth() / WIDTH, (float) getHeight() / HEIGHT);
        float left = (getWidth() - WIDTH * scale) / 2f;
        float x = touchX - left;
        if (x < 0 || x >= WIDTH * scale) return -1;
        return (int) (x / scale / CELL_W) + 1;
    }

    private int touchCellY(float touchY) {
        float scale = Math.min((float) getWidth() / WIDTH, (float) getHeight() / HEIGHT);
        float top = (getHeight() - HEIGHT * scale) / 2f;
        float y = touchY - top;
        if (y < 0 || y >= HEIGHT * scale) return -1;
        return (int) (y / scale / CELL_H) + 1;
    }

    private static final class BoardTransition {
        final byte[] from;
        final byte[] to;
        final short[] order;
        final int type;

        BoardTransition(byte[] from, byte[] to, short[] order, int type) {
            this.from = Arrays.copyOf(from, COLS * ROWS * 2);
            this.to = Arrays.copyOf(to, COLS * ROWS * 2);
            this.order = Arrays.copyOf(order, CELL_COUNT);
            this.type = type;
        }
    }
}

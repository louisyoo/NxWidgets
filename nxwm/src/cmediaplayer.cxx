/********************************************************************************************
 * NxWidgets/nxwm/src/cmediaplayer.cxx
 *
 *   Copyright (C) 2013 Ken Pettit. All rights reserved.
 *   Author: Ken Pettit <pettitkd@gmail.com>
 *           Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX, NxWidgets, nor the names of its contributors
 *    me be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ********************************************************************************************/

/********************************************************************************************
 * Included Files
 ********************************************************************************************/

#include <nuttx/config.h>

#include <cstdio>
#include <debug.h>

#include "cwidgetcontrol.hxx"

#include "nxwmconfig.hxx"
#include "nxwmglyphs.hxx"
#include "cmediaplayer.hxx"

/********************************************************************************************
 * Pre-Processor Definitions
 ********************************************************************************************/

/********************************************************************************************
 * Private Types
 ********************************************************************************************/

/********************************************************************************************
 * Private Data
 ********************************************************************************************/

/********************************************************************************************
 * Private Functions
 ********************************************************************************************/

/********************************************************************************************
 * CMediaPlayer Method Implementations
 ********************************************************************************************/

extern const struct NXWidgets::SRlePaletteBitmap CONFIG_NXWM_MEDIAPLAYER_ICON;

using namespace NxWM;

/**
 * CMediaPlayer constructor
 *
 * @param window.  The application window
 */

CMediaPlayer::CMediaPlayer(CTaskbar *taskbar, CApplicationWindow *window)
{
  // Save the constructor data

  m_taskbar        = taskbar;
  m_window         = window;

  // Nullify widgets that will be instantiated when the window is started

  m_text           = (NXWidgets::CLabel       *)0;
  m_font           = (NXWidgets::CNxFont      *)0;
  m_play           = (NXWidgets::CImage       *)0;
  m_pause          = (NXWidgets::CImage       *)0;
  m_rewind         = (NXWidgets::CStickyImage *)0;
  m_fforward       = (NXWidgets::CStickyImage *)0;
  m_volume         = (NXWidgets::CGlyphSliderHorizontal *)0;

  // Nullify bitmaps that will be instantiated when the window is started

  m_playBitmap     = (NXWidgets::CRlePaletteBitmap *)0;
  m_pauseBitmap    = (NXWidgets::CRlePaletteBitmap *)0;
  m_rewindBitmap   = (NXWidgets::CRlePaletteBitmap *)0;
  m_fforwardBitmap = (NXWidgets::CRlePaletteBitmap *)0;

  // Initial state is stopped

  m_state          = MPLAYER_STOPPED;
  m_prevState      = MPLAYER_STOPPED;
  m_pending        = PENDING_NONE;

  // Add our personalized window label

  NXWidgets::CNxString myName = getName();
  window->setWindowLabel(myName);

  // Add our callbacks with the application window

  window->registerCallbacks(static_cast<IApplicationCallback *>(this));

  // Set the geometry of the media player

  setGeometry();
}

/**
 * CMediaPlayer destructor
 *
 * @param window.  The application window
 */

CMediaPlayer::~CMediaPlayer(void)
{
  // Destroy widgets

  if (m_text)
    {
      delete m_text;
    }

  if (m_font)
    {
      delete m_font;
    }

  if (m_play)
    {
      delete m_play;
    }

  if (m_pause)
    {
      delete m_pause;
    }

  if (m_rewind)
    {
      delete m_rewind;
    }

  if (m_fforward)
    {
      delete m_fforward;
    }

  if (m_volume)
    {
      delete m_volume;
    }

  // Destroy bitmaps

  if (m_playBitmap)
    {
      delete m_playBitmap;
    }

  if (m_pauseBitmap)
    {
      delete m_pauseBitmap;
    }

  if (m_rewindBitmap)
    {
      delete m_rewindBitmap;
    }

  if (m_fforwardBitmap)
    {
      delete m_fforwardBitmap;
    }

  // Although we didn't create it, we are responsible for deleting the
  // application window

  delete m_window;
}

/**
 * Each implementation of IApplication must provide a method to recover
 * the contained CApplicationWindow instance.
 */

IApplicationWindow *CMediaPlayer::getWindow(void) const
{
  return static_cast<IApplicationWindow*>(m_window);
}

/**
 * Get the icon associated with the application
 *
 * @return An instance if IBitmap that may be used to rend the
 *   application's icon.  This is an new IBitmap instance that must
 *   be deleted by the caller when it is no long needed.
 */

NXWidgets::IBitmap *CMediaPlayer::getIcon(void)
{
  NXWidgets::CRlePaletteBitmap *bitmap =
    new NXWidgets::CRlePaletteBitmap(&CONFIG_NXWM_MEDIAPLAYER_ICON);

  return bitmap;
}

/**
 * Get the name string associated with the application
 *
 * @return A copy if CNxString that contains the name of the application.
 */

NXWidgets::CNxString CMediaPlayer::getName(void)
{
  return NXWidgets::CNxString("Media Player");
}

/**
 * Start the application (perhaps in the minimized state).
 *
 * @return True if the application was successfully started.
 */

bool CMediaPlayer::run(void)
{
  // Create the widgets (if we have not already done so)

  if (!m_text)
    {
      // Create the widgets

      if (!createPlayer())
        {
          gdbg("ERROR: Failed to create widgets\n");
          return false;
        }
    }

  return true;
}

/**
 * Stop the application.
 */

void CMediaPlayer::stop(void)
{
  // Just disable further drawing

  m_text->disableDrawing();
}

/**
 * Destroy the application and free all of its resources.  This method
 * will initiate blocking of messages from the NX server.  The server
 * will flush the window message queue and reply with the blocked
 * message.  When the block message is received by CWindowMessenger,
 * it will send the destroy message to the start window task which
 * will, finally, safely delete the application.
 */

void CMediaPlayer::destroy(void)
{
  // Make sure that the widgets are stopped

  stop();

  // Block any further window messages

  m_window->block(this);
}

/**
 * The application window is hidden (either it is minimized or it is
 * maximized, but not at the top of the hierarchy
 */

void CMediaPlayer::hide(void)
{
  // Disable drawing and events

  stop();
}

/**
 * Redraw the entire window.  The application has been maximized or
 * otherwise moved to the top of the hierarchy.  This method is call from
 * CTaskbar when the application window must be displayed
 */

void CMediaPlayer::redraw(void)
{
  // Redraw widgets (only).  Only one of the Play and Pause images should
  // have drawing enabled.

  m_text->redraw();
  m_play->redraw();
  m_pause->redraw();
  m_rewind->redraw();
  m_fforward->redraw();
  m_volume->redraw();
}

/**
 * Report of this is a "normal" window or a full screen window.  The
 * primary purpose of this method is so that window manager will know
 * whether or not it show draw the task bar.
 *
 * @return True if this is a full screen window.
 */

bool CMediaPlayer::isFullScreen(void) const
{
  return m_window->isFullScreen();
}

/**
 * Select the geometry of the media player given the current window size.
 */

void CMediaPlayer::setGeometry(void)
{
  // Recover the NXTK window instance contained in the application window

  NXWidgets::INxWindow *window = m_window->getWindow();

  // Get the size of the window

  (void)window->getSize(&m_windowSize);

  // Get the size of the text box.  Same width as the m_keypad

  m_textSize.w   = m_windowSize.w - 10;
  m_textSize.h   = 36;

  // Now position the text box

  m_textPos.x = 5;
  m_textPos.y = 5;
}

/**
 * Create the media player widgets.  Only start as part of the application
 * start method.
 */

bool CMediaPlayer::createPlayer(void)
{
  // Select a font for the media player

  m_font = new NXWidgets::CNxFont((nx_fontid_e)CONFIG_NXWM_MEDIAPLAYER_FONTID,
                                  CONFIG_NXWM_DEFAULT_FONTCOLOR,
                                  CONFIG_NXWM_TRANSPARENT_COLOR);
  if (!m_font)
    {
      gdbg("ERROR failed to create font\n");
      return false;
    }

  // Get the widget control associated with the application window

  NXWidgets::CWidgetControl *control = m_window->getWidgetControl();

  // Create a label to show some text.  A simple label is used
  // because the power of a text box is un-necessary in this application.

  m_text = new NXWidgets::CLabel(control,
                                 m_textPos.x, m_textPos.y,
                                 m_textSize.w, m_textSize.h,
                                 "0");
  if (!m_text)
    {
      gdbg("ERROR: Failed to create CLabel\n");
      return false;
    }

  // Align text on the left

  m_text->setTextAlignmentHoriz(NXWidgets::CLabel::TEXT_ALIGNMENT_HORIZ_RIGHT);

  // Disable drawing and events until we are asked to redraw the window

  m_text->disableDrawing();
  m_text->setRaisesEvents(false);

  // Select the font

  m_text->setFont(m_font);

  // Add some dummy text for now

  m_text->setText("Coming soon...");

  // Create all bitmaps

  m_playBitmap     = new NXWidgets::CRlePaletteBitmap(&CONFIG_NXWM_MPLAYER_PLAY_ICON);
  m_pauseBitmap    = new NXWidgets::CRlePaletteBitmap(&CONFIG_NXWM_MPLAYER_PAUSE_ICON);
  m_rewindBitmap   = new NXWidgets::CRlePaletteBitmap(&CONFIG_NXWM_MPLAYER_REW_ICON);
  m_fforwardBitmap = new NXWidgets::CRlePaletteBitmap(&CONFIG_NXWM_MPLAYER_FWD_ICON);

  // Image widths will depend on if the images will be bordered or not

  nxgl_coord_t playControlW;
  nxgl_coord_t rewindControlW;
  nxgl_coord_t fforwardControlW;

#ifdef CONFIG_NXWM_MEDIAPLAYER_BORDERS
  // Set the width to the widest image

  nxgl_coord_t imageW = m_playBitmap->getWidth();

  if (imageW < m_pauseBitmap->getWidth())
    {
      imageW = m_pauseBitmap->getWidth();
    }

  if (imageW < m_rewindBitmap->getWidth())
    {
      imageW = m_rewindBitmap->getWidth();
    }

  if (imageW < m_fforwardBitmap->getWidth())
    {
      imageW = m_fforwardBitmap->getWidth();
    }

  // Add little space around the bitmap and use this width for all images

  imageW          += 8;
  playControlW     = imageW;
  rewindControlW   = imageW;
  fforwardControlW = imageW;

#else

  // Use the bitmap image widths for the image widths (plus a bit)

  playControlW     = m_playBitmap->getWidth() + 8;
  rewindControlW   = m_rewindBitmap->getWidth()  + 8;
  fforwardControlW = m_fforwardBitmap->getWidth()  + 8;

  // The Play and Pause images should be the same width.  But just
  // in case, pick the larger width.

  nxgl_coord_t pauseControlW = m_pauseBitmap->getWidth() + 8;
  if (playControlW < pauseControlW)
    {
      playControlW = pauseControlW;
    }
#endif

  // Use the same height for all images

  nxgl_coord_t controlH = m_playBitmap->getHeight();

  if (controlH < m_pauseBitmap->getHeight())
    {
      controlH = m_pauseBitmap->getHeight();
    }

  if (controlH < m_rewindBitmap->getHeight())
    {
      controlH = m_rewindBitmap->getHeight();
    }

  if (controlH < m_fforwardBitmap->getHeight())
    {
      controlH = m_fforwardBitmap->getHeight();
    }

  controlH += 8;

  // Create the Play image

  nxgl_coord_t playControlX = (m_windowSize.w >> 1) - (playControlW >> 1);
  uint32_t controlY         = (180 * m_windowSize.h) >> 8;

  m_play = new NXWidgets::
      CImage(control, playControlX, (nxgl_coord_t)controlY,
             playControlW, controlH, m_playBitmap);

  // Configure the Play image

  m_play->disableDrawing();
  m_play->alignHorizontalCenter();
  m_play->alignVerticalCenter();
#ifndef CONFIG_NXWM_MEDIAPLAYER_BORDERS
  m_play->setBorderless(true);
#else
  m_play->setBorderless(false);
#endif

  // Register to get events from the mouse clicks on the Play image

  m_play->addWidgetEventHandler(this);

  // Create the Pause image (at the same position ans size as the Play image)

  m_pause = new NXWidgets::
      CImage(control, playControlX, (nxgl_coord_t)controlY,
             playControlW, controlH, m_pauseBitmap);

  // Configure the Pause image (hidden and disabled initially)

  m_pause->disableDrawing();
  m_pause->alignHorizontalCenter();
  m_pause->alignVerticalCenter();
#ifndef CONFIG_NXWM_MEDIAPLAYER_BORDERS
  m_pause->setBorderless(true);
#else
  m_pause->setBorderless(false);
#endif

  // Register to get events from the mouse clicks on the Pause image

  m_pause->addWidgetEventHandler(this);

  // Create the Rewind image

  nxgl_coord_t rewControlX = playControlX - rewindControlW -
                             CONFIG_NXWM_MEDIAPLAYER_XSPACING;

  m_rewind = new NXWidgets::
      CStickyImage(control, rewControlX, (nxgl_coord_t)controlY,
                   rewindControlW, controlH, m_rewindBitmap);

  // Configure the Rewind image

  m_rewind->disableDrawing();
  m_rewind->alignHorizontalCenter();
  m_rewind->alignVerticalCenter();
#ifndef CONFIG_NXWM_MEDIAPLAYER_BORDERS
  m_rewind->setBorderless(true);
#else
  m_rewind->setBorderless(false);
#endif

  // Register to get events from the mouse clicks on the Rewind image

  m_rewind->addWidgetEventHandler(this);

  // Create the Forward Image

  nxgl_coord_t fwdControlX = playControlX + playControlW +
                             CONFIG_NXWM_MEDIAPLAYER_XSPACING;

  m_fforward = new NXWidgets::
      CStickyImage(control, fwdControlX, (nxgl_coord_t)controlY,
                   fforwardControlW, controlH, m_fforwardBitmap);

  // Configure the Forward image

  m_fforward->disableDrawing();
  m_fforward->alignHorizontalCenter();
  m_fforward->alignVerticalCenter();
#ifndef CONFIG_NXWM_MEDIAPLAYER_BORDERS
  m_fforward->setBorderless(true);
#else
  m_fforward->setBorderless(false);
#endif

  // Register to get events from the mouse clicks on the Forward image

  m_fforward->addWidgetEventHandler(this);

  // Create the Volume control

  NXWidgets::CRlePaletteBitmap *volBitmap = new NXWidgets::
      CRlePaletteBitmap(&CONFIG_NXWM_MPLAYER_VOL_ICON);

  uint32_t volumeControlX = (9 * m_windowSize.w) >> 8;
  uint32_t volumeControlY = (232 * m_windowSize.h) >> 8;

  m_volume = new NXWidgets::
      CGlyphSliderHorizontal(control,
                             (nxgl_coord_t)volumeControlX,
                             (nxgl_coord_t)volumeControlY,
                             (nxgl_coord_t)(m_windowSize.w - 2 * volumeControlX),
                             volBitmap->getHeight() + 4, volBitmap,
                             MKRGB(63, 90,192));

  // Configure the volume control

  m_volume->disableDrawing();
  m_volume->setMinimumValue(0);
  m_volume->setMaximumValue(100);
  m_volume->setValue(15);

  // Register to get events from the mouse clicks on the Forward image

  m_volume->addWidgetEventHandler(this);

  // Redraw the background once only
  // Get the CCGraphicsPort instance for this window

  NXWidgets::CGraphicsPort *port = control->getGraphicsPort();

  // Fill the entire window with the background color

  port->drawFilledRect(0, 0, m_windowSize.w, m_windowSize.h,
                       CONFIG_NXWM_MEDIAPLAYER_BACKGROUNDCOLOR);

  // Make sure that all widgets are setup for the STOPPED state.  Among other this,
  // this will enable drawing in the play widget (only)

  setMediaPlayerState(MPLAYER_STOPPED);

  // Enable drawing in the text, rewind, fast-forward and drawing widgets.

  m_text->enableDrawing();
  m_rewind->enableDrawing();
  m_fforward->enableDrawing();
  m_volume->enableDrawing();

  // And redraw all of the widgets that are enabled

  redraw();

  return true;
}

/**
 * Called when the window minimize image is pressed.
 */

void CMediaPlayer::minimize(void)
{
  m_taskbar->minimizeApplication(static_cast<IApplication*>(this));
}

/**
 * Called when the window close image is pressed.
 */

void CMediaPlayer::close(void)
{
  m_taskbar->stopApplication(static_cast<IApplication*>(this));
}

/**
 * Transition to a new media player state.
 *
 * @param state The new state to enter.
 */

void CMediaPlayer::setMediaPlayerState(enum EMediaPlayerState state)
{
  // Handle according to the new state

  switch (state)
    {
    case MPLAYER_STOPPED:    // Initial state.  Also the state after playing completes
      m_state     = MPLAYER_STOPPED;
      m_prevState = MPLAYER_PLAYING;

      // Text box is enabled and ready for text entry

      m_text->enable();

      // Play image enabled and ready to start playing

      m_play->enable();
      m_play->show();
      m_play->enableDrawing();

      // Pause image is disabled and hidden

      m_pause->disableDrawing();
      m_pause->disable();
      m_pause->hide();

      // Fast forward image is disabled

      m_fforward->disable();
      m_fforward->setStuckSelection(false);

      // Rewind image is disabled

      m_rewind->disable();
      m_rewind->setStuckSelection(false);

      m_volume->enable();
      break;

    case MPLAYER_PLAYING:    // Playing a media file
      m_state     = MPLAYER_PLAYING;
      m_prevState = MPLAYER_PLAYING;

      // Text box is not available while playing

      m_text->disable();

      // Play image hidden and disabled

      m_play->disableDrawing();
      m_play->disable();
      m_play->hide();

      // Pause image enabled and ready to pause playing

      m_pause->enable();
      m_pause->show();
      m_pause->enableDrawing();

      // Fast forward image is enabled and ready for use

      m_fforward->enable();
      m_fforward->setStuckSelection(false);

      // Rewind image is enabled and ready for use

      m_rewind->enable();
      m_rewind->setStuckSelection(false);

      m_volume->enable();
      break;

    case MPLAYER_PAUSED:     // Playing a media file but paused
      m_state     = MPLAYER_PAUSED;
      m_prevState = MPLAYER_PAUSED;

      // Text box is enabled a ready for text entry

      m_text->enable();

      // Play image enabled and ready to resume playing

      m_play->enable();
      m_play->show();
      m_play->enableDrawing();

      // Pause image is disabled and hidden

      m_pause->disableDrawing();
      m_pause->disable();
      m_pause->hide();

      // Fast forward image is enabled and ready for use

      m_fforward->enable();
      m_fforward->setStuckSelection(false);

      // Rewind image is enabled and ready for use

      m_rewind->enable();
      m_rewind->setStuckSelection(false);

      m_volume->enable();
      break;

    case MPLAYER_FFORWARD:   // Fast forwarding through a media file */
      m_state = MPLAYER_FFORWARD;

      // Text box is not available while fast forwarding

      m_text->disable();

      if (m_prevState == MPLAYER_PLAYING)
        {
          // Play image hidden and disabled

          m_play->disableDrawing();
          m_play->disable();
          m_play->hide();

          // Pause image enabled and ready to stop fast forwarding

          m_pause->enable();
          m_pause->show();
          m_pause->enableDrawing();
        }
      else
        {
          // Play image enabled and ready to stop fast forwarding

          m_play->enable();
          m_play->show();
          m_play->enableDrawing();

          // Pause image is hidden and disabled

          m_pause->disableDrawing();
          m_pause->disable();
          m_pause->hide();
        }

      // Fast forward image is enabled, highlighted and ready for use

      m_fforward->enable();
      m_fforward->setStuckSelection(true);

      // Rewind is enabled and ready for use

      m_rewind->enable();
      m_rewind->setStuckSelection(false);

      m_volume->enable();
      break;

    case MPLAYER_FREWIND:    // Rewinding a media file
      m_state = MPLAYER_FREWIND;

      // Text box is not available while rewinding

      m_text->disable();

      if (m_prevState == MPLAYER_PLAYING)
        {
          // Play image hidden and disabled

          m_play->disableDrawing();
          m_play->disable();
          m_play->hide();

          // Pause image enabled and ready to stop rewinding

          m_pause->enable();
          m_pause->show();
          m_pause->enableDrawing();
        }
      else
        {
          // Play image enabled and ready to stop rewinding

          m_play->enable();
          m_play->show();
          m_play->enableDrawing();

          // Pause image is hidden and disabled

          m_pause->disableDrawing();
          m_pause->disable();
          m_pause->hide();
        }

      // Fast forward image is enabled and ready for use

      m_fforward->enable();
      m_fforward->setStuckSelection(false);

      // Rewind image is enabled, highlighted, and ready for use

      m_rewind->enable();
      m_rewind->setStuckSelection(true);

      m_volume->enable();
      break;

    default:
      break;
    }
}

/**
 * Handle a widget action event.  This includes a image pre/release
 * release events and volume slider change events.
 *
 * @param e The event data.
 */

void CMediaPlayer::handleActionEvent(const NXWidgets::CWidgetEventArgs &e)
{
  // Check if the Play image was clicked

  if (m_play->isClicked() && m_state != MPLAYER_PLAYING)
    {
      // Just arm the state change now, but don't do anything until the
      // release occurs.  Trying to do the state change before the NxWidgets
      // release processing completes causes issues.

      m_pending = PENDING_PLAY_RELEASE;
    }

  // These only make sense in non-STOPPED states

  if (m_state != MPLAYER_STOPPED)
    {
      // Check if the Pause image was clicked

      if (m_pause->isClicked() && m_state != MPLAYER_PAUSED)
        {
         // Just arm the state change now, but don't do anything until the
         // release occurs.  Trying to do the state change before the NxWidgets
         // release processing completes causes issues.

          m_pending = PENDING_PAUSE_RELEASE;
        }

      // Check if the rewind image was clicked

      if (m_rewind->isClicked())
        {
          // Were we already rewinding?

          if (m_state == MPLAYER_FREWIND)
            {
              // Yes.. then revert to the previous play/pause state
              // REVISIT: Or just increase rewind speed?

              setMediaPlayerState(m_prevState);
            }

          // We should not be stopped here, but let's check anyway

          else if (m_state != MPLAYER_STOPPED)
            {
              // Start rewinding

              setMediaPlayerState(MPLAYER_FREWIND);
            }
        }

      // Check if the fast forward image was clicked

      if (m_fforward->isClicked())
        {
          // Were we already fast forwarding?

          if (m_state == MPLAYER_FFORWARD)
            {
              // Yes.. then revert to the previous play/pause state
              // REVISIT: Or just increase fast forward  speed?

              setMediaPlayerState(m_prevState);
            }

          // We should not be stopped here, but let's check anyway

          else if (m_state != MPLAYER_STOPPED)
            {
              // Start fast forwarding

              setMediaPlayerState(MPLAYER_FFORWARD);
            }
        }
    }

  if (m_volume->isClicked())
    {
printf("Volume clicked\n"); // REMOVE ME
    }
}

/**
 * Handle a widget release event.  Only the play and pause image release
 * are of interest.
 */

void CMediaPlayer::handleReleaseEvent(const NXWidgets::CWidgetEventArgs &e)
{
  // Check if the Play image was released

  if (m_pending == PENDING_PLAY_RELEASE && !m_play->isClicked())
    {
      // Now perform the delayed state change

      setMediaPlayerState(MPLAYER_PLAYING);
      m_pending = PENDING_NONE;
    }

  // Check if the Pause image was released

  else if (m_pending == PENDING_PAUSE_RELEASE && !m_pause->isClicked())
    {
      // Now perform the delayed state change

      setMediaPlayerState(MPLAYER_PAUSED);
      m_pending = PENDING_NONE;
    }
}

/**
 * Handle a widget release event when the widget WAS dragged outside of
 * its original bounding box.  Only the play and pause image release
 * are of interest.
 */

void CMediaPlayer::handleReleaseOutsideEvent(const NXWidgets::CWidgetEventArgs &e)
{
  handleReleaseEvent(e);
}

/**
 * CMediaPlayerFactory Constructor
 *
 * @param taskbar.  The taskbar instance used to terminate the console
 */

CMediaPlayerFactory::CMediaPlayerFactory(CTaskbar *taskbar)
{
  m_taskbar = taskbar;
}

/**
 * Create a new instance of an CMediaPlayer (as IApplication).
 */

IApplication *CMediaPlayerFactory::create(void)
{
  // Call CTaskBar::openFullScreenWindow to create a application window for
  // the NxConsole application

  CApplicationWindow *window = m_taskbar->openApplicationWindow();
  if (!window)
    {
      gdbg("ERROR: Failed to create CApplicationWindow\n");
      return (IApplication *)0;
    }

  // Open the window (it is hot in here)

  if (!window->open())
    {
      gdbg("ERROR: Failed to open CApplicationWindow\n");
      delete window;
      return (IApplication *)0;
    }

  // Instantiate the application, providing the window to the application's
  // constructor

  CMediaPlayer *mediaPlayer = new CMediaPlayer(m_taskbar, window);
  if (!mediaPlayer)
    {
      gdbg("ERROR: Failed to instantiate CMediaPlayer\n");
      delete window;
      return (IApplication *)0;
    }

  return static_cast<IApplication*>(mediaPlayer);
}

/**
 * Get the icon associated with the application
 *
 * @return An instance if IBitmap that may be used to rend the
 *   application's icon.  This is an new IBitmap instance that must
 *   be deleted by the caller when it is no long needed.
 */

NXWidgets::IBitmap *CMediaPlayerFactory::getIcon(void)
{
  NXWidgets::CRlePaletteBitmap *bitmap =
    new NXWidgets::CRlePaletteBitmap(&CONFIG_NXWM_MEDIAPLAYER_ICON);

  return bitmap;
}

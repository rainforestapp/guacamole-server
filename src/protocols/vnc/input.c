/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "config.h"

#include "common/cursor.h"
#include "common/display.h"
#include "common/recording.h"
#include "vnc.h"

#include <guacamole/user.h>
#include <rfb/rfbclient.h>

int guac_vnc_user_mouse_handler(guac_user* user, int x, int y, int mask) {

    guac_client* client = user->client;
    guac_vnc_client* vnc_client = (guac_vnc_client*) client->data;
    rfbClient* rfb_client = vnc_client->rfb_client;

    /* Store current mouse location/state */
    guac_common_cursor_update(vnc_client->display->cursor, user, x, y, mask);

    /* Report mouse position within recording */
    if (vnc_client->recording != NULL)
        guac_common_recording_report_mouse(vnc_client->recording, x, y, mask);

    /* Send VNC event only if finished connecting */
    if (rfb_client != NULL)
        SendPointerEvent(rfb_client, x, y, mask);

    return 0;
}

int guac_vnc_user_key_handler(guac_user *user, int keysym, int pressed, int keycode) {

    guac_vnc_client* vnc_client = (guac_vnc_client*) user->client->data;
    /* This variable must be named client so we can use the rfbClientSwap32IfLE
       #defines from libvnc when building our extended key event message */
    rfbClient* client = vnc_client->rfb_client;
    guac_vnc_settings* settings = vnc_client->settings;

    /* Report key state within recording */
    if (vnc_client->recording != NULL)
        guac_common_recording_report_key(vnc_client->recording,
                keysym, pressed);

    /* Send VNC event only if finished connecting */
    if (client != NULL) {
      /* send the keycode as well as the keysym if supported */
      if (settings->ext_qemu_key_events && keycode > 0)
      {
        guac_user_log(user, GUAC_LOG_WARNING, "[qemu ext key message] key event: keysym:%x, keycode:%x, pressed:%x", keysym, keycode, pressed);
        rfbQemuExtKeyEventMsg ke;
        memset(&ke, 0, sizeof(ke));
        ke.type = 255;
        ke.subtype = 0;
        ke.down = pressed ? 1 : 0;
        ke.keysym = rfbClientSwap32IfLE(keysym);
        ke.keycode = rfbClientSwap32IfLE(keycode);
        WriteToRFBServer(client, (char *)&ke, sz_rfbQemuExtKeyEventMsg);
      }
      else /* send just the keysym */
      {
        guac_user_log(user, GUAC_LOG_WARNING, "[key message] key event: keysym:%x, pressed:%x", keysym, pressed);
        SendKeyEvent(client, keysym, pressed);
      }
    }

    return 0;
}


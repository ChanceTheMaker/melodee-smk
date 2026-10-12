/* SPDX-License-Identifier: GPL-3.0-only */
/* Browser-only container. Native bank formats are kept intact; no pointers,
 * audio working buffers, held notes or device flash addresses are serialized. */
#define BROWSER_SESSION_MAGIC 0x32424D53u /* SMB2 */
typedef struct {
    uint32_t magic, version, size, slot, master;
    int32_t octave;
    uint32_t knob_bank, fader_bank;
    uint8_t current[BANK_STORE_SIZE];
    uint8_t projects[4][BANK_STORE_SIZE];
    up_bank_t presets[UP_SLOTS / UP_PER_BANK];
    native_fm_t fm[4];
    cz_bank_t cz[8], legacy_cz[CZ_BANK_N];
    p5_bank_t prophet[5];
    persist_t preferences;
    tmpl_t startup;
    uint32_t checksum;
} browser_session_t;
static browser_session_t session;
_Static_assert(__builtin_offsetof(browser_session_t, checksum) == sizeof session - 4u, "session checksum tail");
_Static_assert(sizeof session < 1000000u, "browser session export limit");
static int session_empty(const void *data, unsigned n)
{
    const uint8_t *p = data;
    for (unsigned i = 0; i < n; i++) if (p[i]) return 0;
    return 1;
}
static int session_settings_valid(const persist_t *p)
{
    return p->magic == PERSIST_MAGIC && palette_stored_ok(p->palette) &&
        p->lowcut <= 2u && (p->zoom & 255u) <= 1u &&
        (!(p->zoom & 0xFFFFFF00u) || (p->zoom & 0xFFFF0000u) == RECORD_PREF_TAG) &&
        hold_stored_ok(p->bold) && p->favorites.filter <= 1u &&
        p->ext.usb_off <= 3u && p->ext.boot <= 4u && p->ext.drumch <= 17u && p->ext.lights <= LIGHTS_N;
}
API uint8_t *fw_snapshot(void)
{
    memset(&session, 0, sizeof session);
    session.magic = BROWSER_SESSION_MAGIC; session.version = 2; session.size = sizeof session;
    session.slot = proj_cur; session.master = song.master_q12; session.octave = song.octave;
    session.knob_bank = smk_knob_bank; session.fader_bank = smk_fader_bank;
    project_capture(&proj_scratch);
    if (!bank_pack(session.current, &proj_scratch, 1)) return 0;
    memcpy(session.projects, proj_bank_slot, sizeof session.projects);
    memcpy(session.presets, up_banks_host, sizeof session.presets);
    memcpy(session.fm, native_fm_host, sizeof session.fm);
    memcpy(session.cz, native_cz_host, sizeof session.cz);
    memcpy(session.legacy_cz, cz_banks_host, sizeof session.legacy_cz);
    memcpy(session.prophet, p5_host, sizeof session.prophet);
    settings_export(&session.preferences);
    session.startup = tmpl;
    session.checksum = proj_hash(&session, sizeof session - 4u);
    return (uint8_t *)&session;
}
API unsigned fw_snapshot_size(void) { return sizeof session; }
API int fw_restore(void)
{
    /* Validate the complete container before replacing any playable state. */
    if (session.magic != BROWSER_SESSION_MAGIC || session.version != 2u || session.size != sizeof session ||
        session.checksum != proj_hash(&session, sizeof session - 4u) ||
        (session.slot >= 4u && session.slot != PROJ_NO_SLOT) || session.master > 4096u ||
        session.octave < -3 || session.octave > 3 || session.knob_bank > 1u || session.fader_bank > 1u ||
        !session_settings_valid(&session.preferences)) return -1;
    if (!session_empty(&session.startup, sizeof session.startup) &&
        !tmpl_blob_valid((const uint8_t *)&session.startup, sizeof session.startup)) return -1;
    for (unsigned i = 0; i < 4; i++) {
        if (!session_empty(session.projects[i], BANK_STORE_SIZE) && !bank_valid(session.projects[i], BANK_STORE_SIZE)) return -1;
        if (!session_empty(&session.fm[i], sizeof session.fm[i]) && !native_fm_valid(&session.fm[i])) return -1;
    }
    for (unsigned i = 0; i < UP_SLOTS / UP_PER_BANK; i++) {
        const up_bank_t *b = &session.presets[i];
        if (session_empty(b, sizeof *b)) continue;
        if (b->magic != UP_BANK_MAGIC || b->rsize != sizeof(up_rec_t) || b->nslot != UP_PER_BANK) return -1;
        /* Native loaders leave unknown record versions inert, as in archive import. */
    }
    for (unsigned i = 0; i < 8; i++) {
        if (!session_empty(&session.cz[i], sizeof session.cz[i]) && !cz_bank_valid(&session.cz[i])) return -1;
        if (!session_empty(&session.legacy_cz[i], sizeof session.legacy_cz[i]) && !cz_bank_valid(&session.legacy_cz[i])) return -1;
    }
    for (unsigned i = 0; i < 5; i++)
        if (!session_empty(&session.prophet[i], sizeof session.prophet[i]) && !p5_bank_valid(&session.prophet[i], i)) return -1;
    /* Last: bank_valid fills the native project scratch and motion tags used below. */
    if (!bank_valid(session.current, BANK_STORE_SIZE)) return -1;
    melodee_smk_release();
    memcpy(proj_bank_slot, session.projects, sizeof session.projects);
    for (unsigned i = 0; i < 4; i++) {
        memset(&proj_slot[i], 0, sizeof proj_slot[i]);
        if (!session_empty(session.projects[i], BANK_STORE_SIZE))
            memcpy(&proj_slot[i], session.projects[i] + 8u, sizeof proj_slot[i]);
        proj_fetch(i); /* Rebuild used flags, names and saved hashes from validated banks. */
    }
    memcpy(up_banks_host, session.presets, sizeof session.presets);
    memcpy(native_fm_host, session.fm, sizeof session.fm);
    memcpy(native_cz_host, session.cz, sizeof session.cz);
    memcpy(cz_banks_host, session.legacy_cz, sizeof session.legacy_cz);
    memcpy(p5_host, session.prophet, sizeof session.prophet);
    memset(native_meta, 0, sizeof native_meta); p5_user_reset();
    cz_bank_cached = native_cz_cached = 255;
    settings_import(&session.preferences, sizeof session.preferences);
    palette_set(settings.palette); perf_latch_on = settings_latch;
    panel = PANEL_DEFAULT; /* SMK mappings are fixed, never imported FM-1 calibration. */
    tmpl = session.startup; tmpl_dirty = 0;
    bank_valid(session.current, BANK_STORE_SIZE); /* proj_fetch reused the native scratch. */
    project_restore_runtime(&proj_scratch); bank_restore(session.current);
    proj_cur = (uint8_t)session.slot;
    song.master_q12 = session.master; song.octave = session.octave;
    smk_knob_bank = session.knob_bank; smk_fader_bank = session.fader_bank;
    ui.force = 1; sync_reload = 1;
    return 0;
}

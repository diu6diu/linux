// SPDX-License-Identifier: GPL-2.0
/*
 * Authors: Wencong Liang <liangwc21@126.com>
 *          
 */
#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>
#include <video/mipi_display.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/drm_print.h>

struct hx8394 {
	struct device *dev;
	struct drm_panel panel;
	struct gpio_desc *reset_gpio;
	struct regulator *supply;
	struct backlight_device *backlight;
	bool prepared;
	bool enabled;
};

static const struct drm_display_mode default_mode = {
    .clock = 65000,
    .hdisplay = 720,
    .hsync_start =  720+ 52,
    .hsync_end = 720 + 52 + 8 ,
    .htotal = 720 + 52 + 8 +48 ,
    .vdisplay = 1280,
    .vsync_start = 1280 + 15,
    .vsync_end = 1280 + 15 + 6,
    .vtotal = 1280 + 15 + 6 + 16,
    .vrefresh = 60,
    .flags = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC,
    .width_mm = 74,
    .height_mm = 150,
};


static inline struct hx8394 *panel_to_hx8394(struct drm_panel *panel)
{
	return container_of(panel, struct hx8394, panel);
}


static void hx8394_dcs_write_buf(struct hx8394 *ctx, const void *data,
				  size_t len)
{
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	int err;

	err = mipi_dsi_dcs_write_buffer(dsi, data, len);
	if (err < 0)
		DRM_ERROR_RATELIMITED("MIPI DSI DCS write buffer failed: %d\n",
				      err);
}

#define dcs_write_seq(ctx, seq...)				\
({								\
	static const u8 d[] = { seq };				\
							    \
	hx8394_dcs_write_buf(ctx, d, ARRAY_SIZE(d));    \
})

static int hx8394_init_sequence(struct hx8394 *ctx)
{
    dcs_write_seq(ctx, 0XB9, 0xFF, 0x83, 0x94);
    dcs_write_seq(ctx, 0X36, 0x1);
    dcs_write_seq(ctx, 0XBA, 0X61, 0X03, 0X68, 0X6B, 0XB2, 0XC0);
    dcs_write_seq(ctx, 0XB1, 0x48, 0x12, 0x72, 0x09, 0x32, 0x54,
                       0x71, 0x71, 0x57, 0x47);
    dcs_write_seq(ctx, 0XB2, 0x00, 0x80, 0x64, 0x0C, 0x0D, 0x2F);
    dcs_write_seq(ctx, 0XB4, 0x73, 0x74, 0x73, 0x74, 0x73, 0x74, 0x01,
                       0x0C, 0x86, 0x75, 0x00, 0x3F, 0x73, 0x74, 0x73, 0x74, 0x73, 0x74, 0x01, 0x0C, 0x86);
    dcs_write_seq(ctx, 0XB6, 0x6E, 0x6E);
    dcs_write_seq(ctx, 0XD3, 0x00, 0x00, 0x07, 0x07, 0x40, 0x07, 0x0C,
                       0x00, 0x08, 0x10, 0x08, 0x00, 0x08, 0x54, 0x15, 0x0A, 0x05, 0x0A, 0x02, 0x15, 0x06,
                       0x05, 0x06, 0x47, 0x44, 0x0A, 0x0A, 0x4B, 0x10, 0x07, 0x07, 0x0C, 0x40);
    dcs_write_seq(ctx, 0XD5, 0x1C, 0x1C, 0x1D, 0x1D, 0x00, 0x01, 0x02,
                       0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x24, 0x25, 0x18, 0x18, 0x26,
                       0x27, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
                       0x18, 0x18, 0x18, 0x20, 0x21, 0x18, 0x18, 0x18, 0x18);
    dcs_write_seq(ctx, 0XD6, 0x1C, 0x1C, 0x1D, 0x1D, 0x07, 0x06, 0x05,
                       0x04, 0x03, 0x02, 0x01, 0x00, 0x0B, 0x0A, 0x09, 0x08, 0x21, 0x20, 0x18, 0x18, 0x27,
                       0x26, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
                       0x18, 0x18, 0x18, 0x25, 0x24, 0x18, 0x18, 0x18, 0x18);
    dcs_write_seq(ctx, 0XE0, 0x00, 0x0A, 0x15, 0x1B, 0x1E, 0x21, 0x24,
                       0x22, 0x47, 0x56, 0x65, 0x66, 0x6E, 0x82, 0x88, 0x8B, 0x9A, 0x9D, 0x98, 0xA8, 0xB9,
                       0x5D, 0x5C, 0x61, 0x66, 0x6A, 0x6F, 0x7F, 0x7F, 0x00, 0x0A, 0x15, 0x1B, 0x1E, 0x21,
                       0x24, 0x22, 0x47, 0x56, 0x65, 0x65, 0x6E, 0x81, 0x87, 0x8B, 0x98, 0x9D, 0x99, 0xA8,
                       0xBA, 0x5D, 0x5D, 0x62, 0x67, 0x6B, 0x72, 0x7F, 0x7F);
    dcs_write_seq(ctx, 0xC0, 0x1F, 0x31);
    dcs_write_seq(ctx, 0XCC, 0x03);
    dcs_write_seq(ctx, 0xD4, 0x02);
    dcs_write_seq(ctx, 0XBD, 0x02);
    dcs_write_seq(ctx, 0xD8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
    dcs_write_seq(ctx, 0XBD, 0x00);
    dcs_write_seq(ctx, 0XBD, 0x01);
    dcs_write_seq(ctx, 0XB1, 0x00);
    dcs_write_seq(ctx, 0XBD, 0x00);
    dcs_write_seq(ctx, 0xBF, 0x40, 0x81, 0x50, 0x00, 0x1A, 0xFC, 0x01);
    dcs_write_seq(ctx, 0xC6, 0xED);

	return 0;
}


static int hx8394_disable(struct drm_panel *panel)
{
    struct hx8394 *ctx = panel_to_hx8394(panel); 
   
	if (!ctx->enabled)
		return 0;

	backlight_disable(ctx->backlight);

	ctx->enabled = false;

	return 0;
}

static int hx8394_unprepare(struct drm_panel *panel)
{
	struct hx8394 *ctx = panel_to_hx8394(panel);
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	int ret;
 
	if (!ctx->prepared)
		return 0;

	ret = mipi_dsi_dcs_set_display_off(dsi);
	if (ret)
		DRM_WARN("failed to set display off: %d\n", ret);

	ret = mipi_dsi_dcs_enter_sleep_mode(dsi);
	if (ret)
		DRM_WARN("failed to enter sleep mode: %d\n", ret);

	msleep(120);

	regulator_disable(ctx->supply);

	ctx->prepared = false;
    
	return 0;
}

static int hx8394_enable(struct drm_panel *panel)
{ 
	struct hx8394 *ctx = panel_to_hx8394(panel);

  if (ctx->enabled)
		return 0;

	ctx->prepared = true;

	return 0;
}

static int hx8394_prepare(struct drm_panel *panel)
{ 
	struct hx8394 *ctx = panel_to_hx8394(panel);
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	int ret;

	if (ctx->enabled)
		return 0;

	ret = regulator_enable(ctx->supply);
	if (ret < 0) {
		DRM_ERROR("failed to enable supply: %d\n", ret);
		return ret;
	}

	hx8394_init_sequence(ctx);

    msleep(120);

	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret)
		return ret;
    
	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret)
		return ret;

	msleep(120);

    backlight_enable(ctx->backlight);
    
	ctx->enabled = true;
    
	return 0;
}

static int hx8394_get_modes(struct drm_panel *panel)
{ 
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(panel->drm, &default_mode);
	if (!mode) {
		DRM_ERROR("failed to add mode %ux%ux@%u\n",
			  default_mode.hdisplay, default_mode.vdisplay,
			  default_mode.vrefresh);
		return -ENOMEM;
	}
	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(panel->connector, mode);

	panel->connector->display_info.width_mm = mode->width_mm;
	panel->connector->display_info.height_mm = mode->height_mm;

	return 1;
}

static const struct drm_panel_funcs hx8394_drm_funcs = {
	.disable = hx8394_disable,
	.unprepare = hx8394_unprepare,
	.prepare = hx8394_prepare,
	.enable = hx8394_enable,
	.get_modes = hx8394_get_modes,
};


static int hx8394_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct hx8394 *ctx;
	int ret;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;
	
	ctx->reset_gpio = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->reset_gpio)) {
		ret = PTR_ERR(ctx->reset_gpio);
		dev_err(dev, "cannot get reset GPIO: %d\n", ret);
		return ret;
	}

	ctx->supply = devm_regulator_get(dev, "power");
	if (IS_ERR(ctx->supply)) {
		ret = PTR_ERR(ctx->supply);
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "cannot get regulator: %d\n", ret);
		return ret;
	}

	ctx->backlight = devm_of_find_backlight(dev);
	if (IS_ERR(ctx->backlight))
		return PTR_ERR(ctx->backlight);

	mipi_dsi_set_drvdata(dsi, ctx);

	ctx->dev = dev;

	dsi->lanes = 2;
	dsi->format = MIPI_DSI_FMT_RGB888;
     dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
              MIPI_DSI_MODE_LPM | MIPI_DSI_CLOCK_NON_CONTINUOUS;

	drm_panel_init(&ctx->panel);
	ctx->panel.dev = dev;
	ctx->panel.funcs = &hx8394_drm_funcs;

	drm_panel_add(&ctx->panel);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		dev_err(dev, "mipi_dsi_attach() failed: %d\n", ret);
		drm_panel_remove(&ctx->panel);
		return ret;
	}    

    return 0;
}

static int hx8394_remove(struct mipi_dsi_device *dsi)
{
	struct hx8394 *ctx = mipi_dsi_get_drvdata(dsi);

	mipi_dsi_detach(dsi);
	drm_panel_remove(&ctx->panel);

	return 0;
}

static const struct of_device_id himax_hx8394_of_match[] = {
	{ .compatible = "himax,hx8394" },
	{ }
};
MODULE_DEVICE_TABLE(of, himax_hx8394_of_match);

static struct mipi_dsi_driver himax_hx8394_driver = {
	.probe = hx8394_probe,
	.remove = hx8394_remove,
	.driver = {
		.name = "panel-himax-hx8394",
		.of_match_table = himax_hx8394_of_match,
	},
};
module_mipi_dsi_driver(himax_hx8394_driver);

MODULE_AUTHOR("Wencong Liang <liangwc21@126.com>");
MODULE_DESCRIPTION("DRM Driver for Himax hx8394 MIPI DSI panel");
MODULE_LICENSE("GPL v2");
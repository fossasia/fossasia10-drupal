
(function ($) {
  Drupal.CTools = Drupal.CTools || {};
  Drupal.CTools.Stylizer = {};

  Drupal.CTools.Stylizer.addFarbtastic = function(context) {
    // This behavior attaches by ID, so is only valid once on a page.
    if ($('ctools_stylizer_color_scheme_form .color-form.Stylizer-processed').size()) {
      return;
    }

    var form = $('.color-form', context);
    var inputs = [];
    var hooks = [];
    var locks = [];
    var focused = null;

    // Add Farbtastic
    $(form).prepend('<div id="placeholder"></div>').addClass('color-processed');
    var farb = $.farbtastic('#placeholder');

    // Decode reference colors to HSL
  /*var reference = Drupal.settings.Stylizer.reference.clone();
    for (i in reference) {
      reference[i] = farb.RGBToHSL(farb.unpack(reference[i]));
    } */

    // Set up colorscheme selector
    $('#edit-scheme', form).change(function () {
      var colors = this.options[this.selectedIndex].value;
      if (colors != '') {
        colors = colors.split(',');
        for (i in colors) {
          callback(inputs[i], colors[i], false, true);
        }
      }
    });

    /**
     * Shift a given color, using a reference pair (ref in HSL).
     *
     * This algorithm ensures relative ordering on the saturation and luminance
     * axes is preserved, and performs a simple hue shift.
     *
     * It is also symmetrical. If: shift_color(c, a, b) == d,
     *                        then shift_color(d, b, a) == c.
     */
    function shift_color(given, ref1, ref2) {
      // Convert to HSL
      given = farb.RGBToHSL(farb.unpack(given));

      // Hue: apply delta
      given[0] += ref2[0] - ref1[0];

      // Saturation: interpolate
      if (ref1[1] == 0 || ref2[1] == 0) {
        given[1] = ref2[1];
      }
      else {
        var d = ref1[1] / ref2[1];
        if (d > 1) {
          given[1] /= d;
        }
        else {
          given[1] = 1 - (1 - given[1]) * d;
        }
      }

      // Luminance: interpolate
      if (ref1[2] == 0 || ref2[2] == 0) {
        given[2] = ref2[2];
      }
      else {
        var d = ref1[2] / ref2[2];
        if (d > 1) {
          given[2] /= d;
        }
        else {
          given[2] = 1 - (1 - given[2]) * d;
        }
      }

      return farb.pack(farb.HSLToRGB(given));
    }

    /**
     * Callback for Farbtastic when a new color is chosen.
     */
    function callback(input, color, propagate, colorscheme) {
      // Set background/foreground color
      $(input).css({
        backgroundColor: color,
        'color': farb.RGBToHSL(farb.unpack(color))[2] > 0.5 ? '#000' : '#fff'
      });

      // Change input value
      if (input.value && input.value != color) {
        input.value = color;

        // Update locked values
        if (propagate) {
          var i = input.i;
          for (j = i + 1; ; ++j) {
            if (!locks[j - 1] || $(locks[j - 1]).is('.unlocked')) break;
            var matched = shift_color(color, reference[input.key], reference[inputs[j].key]);
            callback(inputs[j], matched, false);
          }
          for (j = i - 1; ; --j) {
            if (!locks[j] || $(locks[j]).is('.unlocked')) break;
            var matched = shift_color(color, reference[input.key], reference[inputs[j].key]);
            callback(inputs[j], matched, false);
          }

        }

        // Reset colorscheme selector
        if (!colorscheme) {
          resetScheme();
        }
      }

    }

    /**
     * Reset the color scheme selector.
     */
    function resetScheme() {
      $('#edit-scheme', form).each(function () {
        this.selectedIndex = this.options.length - 1;
      });
    }

    // Focus the Farbtastic on a particular field.
    function focus() {
      var input = this;
      // Remove old bindings
      focused && $(focused).unbind('keyup', farb.updateValue)
          .unbind('keyup', resetScheme)
          .parent().removeClass('item-selected');

      // Add new bindings
      focused = this;
      farb.linkTo(function (color) { callback(input, color, true, false); });
      farb.setColor(this.value);
      $(focused).keyup(farb.updateValue).keyup(resetScheme)
        .parent().addClass('item-selected');
    }

    // Initialize color fields
    $('#palette input.form-text', form)
    .each(function () {
      // Extract palette field name
      this.key = this.id.substring(13);

      // Link to color picker temporarily to initialize.
      farb.linkTo(function () {}).setColor('#000').linkTo(this);

      // Add lock
      var i = inputs.length;
      if (inputs.length) {
        var lock = $('<div class="lock"></div>').toggle(
          function () {
            $(this).addClass('unlocked');
            $(hooks[i - 1]).attr('class',
              locks[i - 2] && $(locks[i - 2]).is(':not(.unlocked)') ? 'hook up' : 'hook'
            );
            $(hooks[i]).attr('class',
              locks[i] && $(locks[i]).is(':not(.unlocked)') ? 'hook down' : 'hook'
            );
          },
          function () {
            $(this).removeClass('unlocked');
            $(hooks[i - 1]).attr('class',
              locks[i - 2] && $(locks[i - 2]).is(':not(.unlocked)') ? 'hook both' : 'hook down'
            );
            $(hooks[i]).attr('class',
              locks[i] && $(locks[i]).is(':not(.unlocked)') ? 'hook both' : 'hook up'
            );
          }
        );
        $(this).after(lock);
        locks.push(lock);
      };

      // Add hook
      var hook = $('<div class="hook"></div>');
      $(this).after(hook);
      hooks.push(hook);

      $(this).parent().find('.lock').click();
      this.i = i;
      inputs.push(this);
    })
    .focus(focus);

    $('#palette label', form);

    // Focus first color
    focus.call(inputs[0]);
  };

  Drupal.behaviors.CToolsColorSettings = function() {
    $('.ctools-stylizer-color-edit:not(.ctools-color-processed)')
      .addClass('ctools-color-processed')
      .each(function() {
        Drupal.CTools.Stylizer.addFarbtastic('#' + $(this).attr('id'));
      });

    $('div.form-item div.ctools-style-icon:not(.ctools-color-processed)')
      .addClass('ctools-color-processed')
      .click(function() {
        $widget = $('input', $(this).parent());
        // Toggle if a checkbox, turn on if a radio.
        $widget.attr('checked', !$widget.attr('checked') || $widget.is('input[type=radio]'));
      });
  }
})(jQuery);

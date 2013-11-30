Drupal.behaviors.LoginToboggan = function (context) {
  $("#toboggan-login:not(.toboggan-login-processed)", context).each(
    function() {
      $(this).addClass('toboggan-login-processed').hide();
      Drupal.logintoboggan_toggleboggan();
    }
  );
};

Drupal.logintoboggan_toggleboggan = function() {
  $("#toboggan-login-link").click(
    function () {
      $("#toboggan-login").slideToggle("fast");
      this.blur();
      return false;
    }
  );
};
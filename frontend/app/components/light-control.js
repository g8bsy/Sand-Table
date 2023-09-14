import Component from '@glimmer/component';
import { action } from '@ember/object';
import { tracked } from '@glimmer/tracking';

export default class LightControlComponent extends Component {
  components = {
    opacity: false,
    hue: true,
    interaction: {
      hex: false,
      rgba: false,
      hsva: false,
      input: false,
    },
  };

  methods = [
    'OFF',
    'COLOR_WIPE',
    'THEATRE_CHASE',
    'RAINBOW',
    'RAINBOW_CYCLE',
    'RAINBOW_THEATRE_CHASE',
  ];

  constructor(args) {
    super(...arguments);
    this.loadData();
  }

  @tracked color = null;
  @tracked method = null;
  @tracked brightness = null;

  @action
  chooseBrightness(brightness) {
    this.brightness = brightness.srcElement.value;
    $.ajax({
      type: 'GET',
      url: '/api/led/put_brightness/' + this.brightness,
      async: false,
    });
  }

  @action
  chooseMethod(method) {
    this.method = method;
    $.ajax({
      type: 'GET',
      url: '/api/led/put_method/' + method,
      async: false,
    });
  }

  @action onSave(hsva, instance) {
    var tha = hsva.toHEXA();
    var hex = tha[0] + tha[1] + tha[2];
    $.ajax({
      type: 'GET',
      url: '/api/led/put_color/' + hex.toUpperCase(),
      async: false,
    });
    instance.hide();
  }

  loadData() {
    var led_cfg = JSON.parse(
      $.ajax({
        type: 'GET',
        url: '/api/led/cfg',
        async: false,
      }).responseText
    );

    this.color = led_cfg.color;
    this.method = led_cfg.method;
    this.brightness = led_cfg.brightness;
  }
}

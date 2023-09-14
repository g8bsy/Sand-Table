import Component from '@glimmer/component';
import { action } from '@ember/object';
import { tracked } from '@glimmer/tracking';

export default class ControlPanelComponent extends Component {
  @tracked speed;

  constructor(args) {
    super(...arguments);
    this.loadData();
  }

  @action chooseSpeed(speed) {
    this.speed = speed.srcElement.value;
    $.ajax({
      type: 'GET',
      url: '/api/put_speed/' + this.speed,
      async: false,
    });
  }

  loadData() {
    var cfg = JSON.parse(
      $.ajax({
        type: 'GET',
        url: '/api/speed',
        async: false,
      }).responseText
    );

    this.speed = cfg.speed;
  }
}

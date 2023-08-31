import Component from '@glimmer/component';
import { action } from '@ember/object';
import { tracked } from '@glimmer/tracking';

export default class TrackListComponent extends Component {
  @tracked mightPlay = null;
  @tracked playImmediately = false;
  @tracked eraseFirst = true;

  @action setMightPlay(item) {
    this.mightPlay = item;
  }

  @action queueTrack() {
    $.ajax({
      type: 'GET',
      url: '/api/run_file/' + this.mightPlay.id,
      async: false,
    });
    this.mightPlay = null;
  }
}

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

  @action queue()
}

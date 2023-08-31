import Route from '@ember/routing/route';

export default class IndexRoute extends Route {
  async model() {
    return {
      files: JSON.parse(
        $.ajax({
          type: 'GET',
          url: '/api/files',
          async: false,
        }).responseText
      ),
    };
  }
}

from django.urls import path
from .views import ImageProcessor

urlpatterns = [
    path('upload/', ImageProcessor.as_view(), name='upload_image'),
]
